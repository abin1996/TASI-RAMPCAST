import json
import requests
from datetime import datetime
import time

def write_graphql_json_to_file(json_data, filename):
    
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    filename = "data_"+filename + "_" +timestamp + ".json"

    response = requests.post('https://511in.org/api/graphql', json=json_data)
    print(json.dumps(response.json(), indent=2))

    with open(filename, "w") as write_file:
        json.dump(response.json(), write_file)

def dump_incidents():

    json_data_incidents = {
        "query": "query MapFeatures($input: MapFeaturesArgs!, $plowType: String) { mapFeaturesQuery(input: $input) { mapFeatures { bbox tooltip uri features { id geometry properties } __typename ... on Camera { views(limit: 5) { uri ... on CameraView { url } category } } ... on Plow { views(limit: 5, plowType: $plowType) { uri ... on PlowCameraView { url } category } } } error { message type } } }",
        "variables": {
            "input": {
                "north": 41.58436,
                "south": 37.4026,
                "east": -82.85645,
                "west": -88.48145,
                "zoom": 8,
                "layerSlugs": [
                    "incidents"
                ],
                "nonClusterableUris": [
                    "dashboard"
                ]
            },
            "plowType": "plowCameras"
        }
    }

    

    write_graphql_json_to_file(json_data_incidents, "incidents")


def dump_electronic_signs():

    json_data_signs = {
        "query": "query MapFeatures($input: MapFeaturesArgs!, $plowType: String) { mapFeaturesQuery(input: $input) { mapFeatures { bbox tooltip uri features { id geometry properties } __typename ... on Camera { views(limit: 5) { uri ... on CameraView { url } category } } ... on Plow { views(limit: 5, plowType: $plowType) { uri ... on PlowCameraView { url } category } } } error { message type } } }",
        "variables": {
            "input": {
                "north": 41.58436,
                "south": 37.4026,
                "east": -82.85645,
                "west": -88.48145,
                "zoom": 8,
                "layerSlugs": [
                    "electronicSigns"
                ],
                "nonClusterableUris": [
                    "dashboard"
                ]
            },
            "plowType": "plowCameras"
        }
    }

    write_graphql_json_to_file(json_data_signs, "e-Signs")

def dump_construction_warnings():

    json_data_signs =     {
        "query": "query MapFeatures($input: MapFeaturesArgs!, $plowType: String) { mapFeaturesQuery(input: $input) { mapFeatures { bbox tooltip uri features { id geometry properties } __typename ... on Camera { views(limit: 5) { uri ... on CameraView { url } category } } ... on Plow { views(limit: 5, plowType: $plowType) { uri ... on PlowCameraView { url } category } } } error { message type } } }",
        "variables": {
            "input": {
                "north": 41.58436,
                "south": 37.4026,
                "east": -82.85645,
                "west": -88.48145,
                "zoom": 8,
                "layerSlugs": [
                    "construction"
                ],
                "nonClusterableUris": [
                    "electronic-sign/indianasigns*1522"
                ]
            },
            "plowType": "plowCameras"
        }
    }
    
    write_graphql_json_to_file(json_data_signs, "construction_warnings")
    
def main():

    while True:

        dump_incidents()
        dump_electronic_signs()
        dump_construction_warnings()

        time.sleep(3600) # wait for 1 hour for next dump 
    

if __name__ == "__main__":
    main()
