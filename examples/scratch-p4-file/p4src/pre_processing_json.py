# Script, which can search the content in json files and replace strings:
# The string that needs to be replaced is:
#
#            "op" : "mark_to_drop",
#            "parameters" : [
#              {
#                "type" : "header",
#                "value" : "standard_metadata"
#              }
#            ],
#
# Replace with string:
#            "op" : "drop",
#            "parameters" : [],

import json
import sys

def replace_data(data):
    if isinstance(data, list):
        for i, item in enumerate(data):
            data[i] = replace_data(item)
    elif isinstance(data, dict):
        if data.get("op") == "mark_to_drop" and "parameters" in data and isinstance(data["parameters"], list):
            found = False
            for param in data["parameters"]:
                if param.get("type") == "header" and param.get("value") == "standard_metadata":
                    found = True
                    break
            if found:
                data["op"] = "drop"
                data["parameters"] = []
        for key in data:
            data[key] = replace_data(data[key])
    return data

def main(json_path):
    with open(json_path, "r") as file:
        data = json.load(file)

    modified_data = replace_data(data)

    with open(json_path, "w") as file:
        json.dump(modified_data, file, indent=4)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script_name.py <path_to_json_file>")
        sys.exit(1)
    main(sys.argv[1])
