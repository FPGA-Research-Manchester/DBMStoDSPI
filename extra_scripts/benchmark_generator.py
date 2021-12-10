import numpy as np
from enum import Enum
import json


class State(Enum):
    filter = 1
    join = 2
    arithmetic = 3
    aggregation = 4
    finish = 5


node_counter = 0
table_counter = 0


def random_selection(integer_limit):
    return np.random.randint(integer_limit)


def get_filter(current_graph, before_nodes, current_join_nodes):
    global node_counter
    if (random_selection(2)):
        table_list = check_for_dependency(current_graph, before_nodes)
        current_graph["node" + str(node_counter)] = {"operation": "Filter", "capacity": [random_selection(10), random_selection(
            10)], "before": before_nodes.copy(), "after": [], "tables": table_list, "satisfying_bitstreams": []}
        # Assuming len is 1
        before_nodes[0] = ["node" + str(node_counter), 0]
        node_counter += 1
        if(current_join_nodes and random_selection(2)):
            removed_element = current_join_nodes.pop(
                random_selection(len(current_join_nodes)))
            current_graph[before_nodes[0]]["after"].append(
                removed_element)
            current_graph[removed_element]["before"].append(
                [before_nodes[0], 0])
            current_graph[removed_element]["tables"].append("")
            return State.finish
    return State.join


def get_join(current_graph, before_nodes, next_join_nodes):
    global table_counter
    global node_counter
    if (random_selection(2)):
        table_list = check_for_dependency(current_graph, before_nodes)
        current_graph["node" + str(node_counter)] = {"operation": "Linear Sort", "capacity": [], "before": before_nodes.copy(), "after": [
            "node" + str(node_counter+1)], "tables": table_list, "satisfying_bitstreams": []}
        node_counter += 1
        current_graph["node" + str(node_counter)] = {"operation": "Merge Sort", "capacity": [], "before": [["node" + str(
            node_counter-1), 0]], "after": ["node" + str(node_counter+1)], "tables": [""], "satisfying_bitstreams": []}
        node_counter += 1

        next_join_nodes.append("node" + str(node_counter))

        current_graph["node" + str(node_counter)] = {"operation": "Linear Sort", "capacity": [], "before": [
        ], "after": ["node" + str(node_counter+1)], "tables": [], "satisfying_bitstreams": []}
        node_counter += 1
        current_graph["node" + str(node_counter)] = {"operation": "Merge Sort", "capacity": [], "before": [["node" + str(
            node_counter-1), 0]], "after": ["node" + str(node_counter+1)], "tables": [""], "satisfying_bitstreams": []}
        node_counter += 1
        current_graph["node" + str(node_counter)] = {"operation": "Merge Join", "capacity": [], "before": [["node" + str(
            node_counter-3), 0], ["node" + str(node_counter-1), 0]], "after": [], "tables": ["", ""], "satisfying_bitstreams": []}
        # Assuming len is 1
        before_nodes[0] = ["node" + str(node_counter), 0]
        node_counter += 1
        return State.filter
    return State.arithmetic


def get_arithmetic(current_graph, before_nodes):
    global node_counter
    if (random_selection(2)):
        table_list = check_for_dependency(current_graph, before_nodes)
        operation = "Multiplier"
        if (random_selection(2)):
            operation = "Addition"
        current_graph["node" + str(node_counter)] = {"operation": operation, "capacity": [
        ], "before": before_nodes.copy(), "after": [], "tables": table_list, "satisfying_bitstreams": []}
        # Assuming len is 1
        before_nodes[0] = ["node" + str(node_counter), 0]
        node_counter += 1
        return State.arithmetic
    return State.aggregation


def get_aggregation(current_graph, before_nodes):
    global node_counter
    if (random_selection(2)):
        table_list = check_for_dependency(current_graph, before_nodes)
        current_graph["node" + str(node_counter)] = {"operation": "Global Sum", "capacity": [
        ], "before": before_nodes.copy(), "after": [], "tables": table_list, "satisfying_bitstreams": []}
        # Assuming len is 1
        before_nodes[0] = ["node" + str(node_counter), 0]
        node_counter += 1
    return State.finish


def check_for_dependency(current_graph, before_nodes):
    global table_counter
    table_list = []
    if (len(before_nodes) == 1 and before_nodes[0] == ["", -1]):
        table_list.append("table_" + str(table_counter))
        table_counter += 1
    else:
        table_list.append("")
        # Assuming len is 1
        current_graph[before_nodes[0][0]]["after"].append(
            "node" + str(node_counter))
    return table_list


def main():
    query_count = 1
    current_graph = {}
    current_join_nodes = []
    for i in range(query_count):
        next_join_nodes = []
        before_nodes = [["", -1]]
        current_state = State.filter
        while (current_state != State.finish):
            # Simple classless FSM for now
            if (current_state == State.filter):
                current_state = get_filter(
                    current_graph, before_nodes, current_join_nodes)
            elif (current_state == State.join):
                current_state = get_join(
                    current_graph, before_nodes, next_join_nodes)
            elif (current_state == State.arithmetic):
                current_state = get_arithmetic(current_graph, before_nodes)
            elif (current_state == State.aggregation):
                current_state = get_aggregation(current_graph, before_nodes)
        current_join_nodes.extend(next_join_nodes)
        if (before_nodes[0] != ["", -1] and not current_graph[before_nodes[0][0]]["after"]):
            current_graph[before_nodes[0][0]]["after"].append("")

    # For all current join fix the table
    global table_counter
    for node in current_join_nodes:
        current_graph[node]["tables"].append("table_" + str(table_counter))
        table_counter += 1
        current_graph[node]["before"].append(["", -1])

    with open('input_graph.json', 'w', encoding='utf-8') as graph_json:
        json.dump(current_graph, graph_json, ensure_ascii=False, indent=4)

    table_data = {}
    for i in range(table_counter):
        table_data["table_" +
                   str(i)] = {"record_count": random_selection(10000), "sorted_sequences": []}

    with open('input_tables.json', 'w', encoding='utf-8') as table_json:
        json.dump(table_data, table_json, ensure_ascii=False, indent=4)


if __name__ == '__main__':
    main()
