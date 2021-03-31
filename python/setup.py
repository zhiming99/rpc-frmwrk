from setuptools import setup, find_packages
import importlib.machinery
extlib ='rpcbase' + importlib.machinery.EXTENSION_SUFFIXES[0]
print( extlib )
kwargs = {
      'name':'rpcf',
      'version':'0.1.0',
      'packages': find_packages(),

       #ensure so-files are copied to the installation:
      'package_data' : { 'rpcf': ['proxy',extlib]},
      'include_package_data' : True,
      'zip_safe' : False  
}

setup(**kwargs)

