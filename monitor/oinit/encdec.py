import os
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric import padding, rsa, ec
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.serialization import load_pem_public_key, load_pem_private_key
from cryptography.x509 import load_pem_x509_certificate 
from pathlib import Path
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

def Encrypt_Bytes(data: bytes) -> bytes:
    strHome = str(Path.home()) + "/.rpcf/openssl/"
    certs = ['clientcert.pem', "signcert.pem"]
    encrypted = None

    for i in certs:
        try:
            with open(strHome + i, 'rb') as cert_file:
                cert = load_pem_x509_certificate(cert_file.read())
                public_key = cert.public_key()

                if isinstance(public_key, rsa.RSAPublicKey):
                    # RSA direct encryption
                    encrypted = public_key.encrypt(
                        data,
                        padding.OAEP(
                            mgf=padding.MGF1(algorithm=hashes.SHA256()),
                            algorithm=hashes.SHA256(),
                            label=None
                        )
                    )
                elif isinstance(public_key, ec.EllipticCurvePublicKey):
                    # EC: Hybrid encryption (ECDH + AES)
                    # Generate ephemeral EC private key
                    ephemeral_key = ec.generate_private_key(public_key.curve, default_backend())
                    shared_key = ephemeral_key.exchange(ec.ECDH(), public_key)
                    # Derive AES key from shared secret
                    aes_key = HKDF(
                        algorithm=hashes.SHA256(),
                        length=32,
                        salt=None,
                        info=b'ecdh-encryption',
                        backend=default_backend()
                    ).derive(shared_key)
                    # Encrypt data with AES-GCM
                    iv = os.urandom(12)
                    encryptor = Cipher(
                        algorithms.AES(aes_key),
                        modes.GCM(iv),
                        backend=default_backend()
                    ).encryptor()
                    ciphertext = encryptor.update(data) + encryptor.finalize()
                    tag = encryptor.tag
                    # Serialize ephemeral public key
                    ephemeral_pub_bytes = ephemeral_key.public_key().public_bytes(
                        encoding=serialization.Encoding.DER,
                        format=serialization.PublicFormat.SubjectPublicKeyInfo
                    )
                    # Output: ephemeral_pub | iv | tag | ciphertext
                    encrypted = ephemeral_pub_bytes + iv + tag + ciphertext
                else:
                    raise NotImplementedError("Unsupported public key type")
                break
        except Exception as err:
            print(err)
            continue
    return encrypted

def Decrypt_Bytes(encrypted: bytes) -> bytes:
    strHome = str(Path.home()) + "/.rpcf/openssl/"
    private_keys = ['clientkey.pem', "signkey.pem"]
    decrypted = None

    for i in private_keys:
        try:
            with open(strHome + i, 'rb') as private_key_file:
                private_key = load_pem_private_key(
                    private_key_file.read(), password=None,
                    backend=default_backend())

            if isinstance(private_key, rsa.RSAPrivateKey):
                # RSA direct decryption
                decrypted = private_key.decrypt(
                    encrypted,
                    padding.OAEP(
                        mgf=padding.MGF1(algorithm=hashes.SHA256()),
                        algorithm=hashes.SHA256(),
                        label=None
                    )
                )
            elif isinstance(private_key, ec.EllipticCurvePrivateKey):
                # EC: Hybrid decryption (ECDH + AES)
                # Parse ephemeral public key, iv, tag, ciphertext
                # (Assume DER-encoded ephemeral pubkey is 91 bytes for P-256, adjust as needed)
                curve = private_key.curve
                if isinstance(curve, ec.SECP256R1):
                    pubkey_len = 91
                elif isinstance(curve, ec.SECP384R1):
                    pubkey_len = 120
                elif isinstance(curve, ec.SECP521R1):
                    pubkey_len = 158
                else:
                    raise NotImplementedError("Unknown EC curve")
                ephemeral_pub_bytes = encrypted[:pubkey_len]
                iv = encrypted[pubkey_len:pubkey_len+12]
                tag = encrypted[pubkey_len+12:pubkey_len+28]
                ciphertext = encrypted[pubkey_len+28:]

                ephemeral_pub = load_pem_public_key(
                    serialization.Encoding.DER.encode(ephemeral_pub_bytes),
                    backend=default_backend()
                )
                shared_key = private_key.exchange(ec.ECDH(), ephemeral_pub)
                aes_key = HKDF(
                    algorithm=hashes.SHA256(),
                    length=32,
                    salt=None,
                    info=b'ecdh-encryption',
                    backend=default_backend()
                ).derive(shared_key)
                decryptor = Cipher(
                    algorithms.AES(aes_key),
                    modes.GCM(iv, tag),
                    backend=default_backend()
                ).decryptor()
                decrypted = decryptor.update(ciphertext) + decryptor.finalize()
            else:
                raise NotImplementedError("Unsupported private key type")
            break
        except Exception as err:
            print(err)
            continue
    return decrypted
