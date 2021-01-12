import json, os

def check_utilisation(path_to_reports):
    with open(path_to_reports + '/lib/json/summary.json', 'r') as f:
        report = json.load(f)

    estimatedResources = report["estimatedResources"]
    columns = estimatedResources['columns'][1:]
    for child in estimatedResources["children"]:
        if child['name'] == 'Total':
            percentages = child['data_percent']
            totals = child['data']
            break
    percentages.append(0.0)

    utilisation = {}
    for i in range(len(columns)):
        utilisation[columns[i]] = {'percentage': percentages[i], 'total': totals[i]}
    # print(columns)
    return utilisation

# check_utilisation(os.getcwd() + '/swi_project/project/bin/kernel/reports')