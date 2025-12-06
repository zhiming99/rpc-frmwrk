#!/bin/python3
import json

def swap_proxy_port_classes(file_path):
    # Load the JSON file
    with open(file_path, 'r') as file:
        data = json.load(file)

    # Iterate through the "Objects" list and swap the values
    for obj in data.get("Objects", []):
        # Skip the swap if the ObjectName is "AppMonitor"
        strName = obj.get( "ObjectName" )
        if( strName == "AppMonitor" or
            strName == "AppMonitor_ChannelSvr" ):
            continue

        if "ProxyPortClass" in obj and "ProxyPortClass-1" in obj:
            obj["ProxyPortClass"], obj["ProxyPortClass-1"] = obj["ProxyPortClass-1"], obj["ProxyPortClass"]

    # Save the updated JSON back to the file
    with open(file_path, 'w') as file:
        json.dump(data, file, indent=4)

    print(f"Swapped 'ProxyPortClass' and 'ProxyPortClass-1' in {file_path}, excluding 'AppMonitor' objects.")

# Specify the path to the JSON file
file_path = "appmondesc.json"

# Call the function
swap_proxy_port_classes(file_path)
