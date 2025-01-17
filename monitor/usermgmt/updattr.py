import json
import os
import fcntl
import argparse

def setJsonXattr(filePath, attrName, data):
    with open(filePath, 'w') as file:
        fcntl.flock(file, fcntl.LOCK_EX)
        os.setxattr(filePath, attrName, data.encode('utf-8'))
        fcntl.flock(file, fcntl.LOCK_UN)

def listXattrs(filePath):
    attrs = os.listxattr(filePath)
    for attr in attrs:
        value = os.getxattr(filePath, attr).decode('utf-8')
        print(f"{attr}: {value}")

def updateJsonXattr(filePath, attrName, updates):
    if not os.path.exists(filePath):
        raise FileNotFoundError(f"The file {filePath} does not exist.")
    
    setJsonXattr(filePath, attrName, updates)

def addIntegerXattr(filePath, attrName, value)->int:
    with open(filePath, 'r+') as file:
        fcntl.flock(file, fcntl.LOCK_EX)
        jsonData = os.getxattr(filePath, attrName).decode('utf-8')
        jsonValue = json.loads(jsonData)
        if 't' not in jsonValue:
            raise ValueError(f"The attribute {attrName} is not an expected variant object.")
        if jsonValue['t'] != 3:
            raise ValueError(f"The attribute {attrName} is not the integer type.")
        if 'v' not in jsonValue:
            raise ValueError(f"The attribute {attrName} does not have a value.")
        currentValue = int(jsonValue['v'])
        newValue = currentValue + value
        jsonValue['v'] = newValue
        jsonData = json.dumps(jsonValue).encode('utf-8')
        os.setxattr(filePath, attrName, jsonData)
        fcntl.flock(file, fcntl.LOCK_UN)
        return currentValue

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Manage JSON data in file extended attributes.")
    parser.add_argument('filePath', help="Path to the file.")
    parser.add_argument('-l', '--list', action='store_true', help="List all attributes and their values.")
    parser.add_argument('-u', '--update', nargs=2, metavar=('ATTR_NAME', 'UPDATES'), help="Update a JSON attribute.")
    parser.add_argument('-a', '--add', nargs=2, metavar=('ATTR_NAME', 'VALUE'), type=str, help="Add an integer value to an attribute.")

    args = parser.parse_args()

    if args.list:
        listXattrs(args.filePath)
    elif args.update:
        attrName, updates = args.update
        updateJsonXattr(args.filePath, attrName, updates)
        print(f"Updated {args.filePath} successfully.")
    elif args.add:
        attrName, value = args.add
        ret = addIntegerXattr(args.filePath, attrName, int( value) )
        print( ret )
    else:
        parser.print_help()