import os
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from cryptography.hazmat.primitives.serialization import load_pem_private_key
from cryptography.x509 import load_pem_x509_certificate 
from cryptography.hazmat.primitives import hashes
from pathlib import Path

def Encrypt_Bytes( data : bytearray )->bytes:
    strHome = str( Path.home() ) + "/.rpcf/openssl/"
    certs = [ 'clientcert.pem',  "signcert.pem" ]
    for i in certs:
        try:
            with open(strHome + i, 'rb') as cert_file:
                cert = load_pem_x509_certificate(cert_file.read())
                public_key = cert.public_key()
                encrypted = public_key.encrypt(
                    data,
                    padding.OAEP(
                        mgf=padding.MGF1(algorithm=hashes.SHA256()),
                        algorithm=hashes.SHA256(),
                        label=None
                    )
                )
                break
        except Exception as err:
            print( err )
            continue
    return encrypted
 
def Decrypt_Bytes( encrypted : bytearray )->bytes:
    strHome = str( Path.home() ) + "/.rpcf/openssl/"
    private_keys = [ 'clientkey.pem',  "signkey.pem" ]

    for i in private_keys:
        try:
            with open(strHome + i, 'rb') as private_key_file:
                private_key = load_pem_private_key(
                    private_key_file.read(), password=None,
                    backend=default_backend())
            
            decrypted = private_key.decrypt(
                encrypted,
                padding.OAEP(
                    mgf=padding.MGF1(algorithm=hashes.SHA256()),
                    algorithm=hashes.SHA256(),
                    label=None
                )
            )
        except Exception as err:
            continue
    return decrypted