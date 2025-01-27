# Copyright 2021 University of Manchester
#
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http:  // www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


from dataclasses import dataclass
from typing import Tuple
from sys import maxsize
from enum import Enum
from time import perf_counter
import json
import copy
import sys

from numpy import mod


# ----------- Module Placement Struct -----------
@dataclass(eq=True, frozen=True)
class ScheduledModule:
    node_name: str
    operation: str
    bitstream: str
    position: Tuple[int, int]  # start and end pos (inclusive)


# ----------- Pretty Print Strings -----------
class FancyText:
    PURPLE = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    DEFAULT = '\033[39m'
    END = '\033[0m'


# ----------- Module Selection Enums -----------
def select_all_placements_enum_func(available_module_placements):
    return available_module_placements


def select_first_placement_enum_func(available_module_placements):
    chosen_module_placements = []
    min_available_position = maxsize
    for placement in available_module_placements:
        if placement[1].position[0] < min_available_position:
            min_available_position = placement[1].position[0]
    for placement in available_module_placements:
        if placement[1].position[0] == min_available_position and placement not in chosen_module_placements:
            chosen_module_placements.append(placement)
    return chosen_module_placements


def select_last_placement_enum_func(available_module_placements):
    chosen_module_placements = []
    max_available_position = 0
    for placement in available_module_placements:
        if placement[1].position[0] > max_available_position:
            max_available_position = placement[1].position[0]
    for placement in available_module_placements:
        if placement[1].position[0] == max_available_position and placement not in chosen_module_placements:
            chosen_module_placements.append(placement)
    return chosen_module_placements


def select_shortest_placement_enum_func(available_module_placements):
    chosen_module_placements = []
    min_module_size = maxsize
    for placement in available_module_placements:
        if placement[1].position[1] - \
                placement[1].position[0] + 1 < min_module_size:
            min_module_size = placement[1].position[1] - \
                placement[1].position[0] + 1
    for placement in available_module_placements:
        if placement[1].position[1] - placement[1].position[0] + 1 == min_module_size \
                and placement not in chosen_module_placements:
            chosen_module_placements.append(placement)
    return chosen_module_placements


def select_longest_placement_enum_func(available_module_placements):
    chosen_module_placements = []
    max_module_size = 0
    for placement in available_module_placements:
        if placement[1].position[1] - \
                placement[1].position[0] + 1 > max_module_size:
            max_module_size = placement[1].position[1] - \
                placement[1].position[0] + 1
    for placement in available_module_placements:
        if placement[1].position[1] - placement[1].position[0] + 1 == max_module_size \
                and placement not in chosen_module_placements:
            chosen_module_placements.append(placement)
    return chosen_module_placements


class ModuleSelection(Enum):
    ALL_AVAILABLE = (select_all_placements_enum_func,)
    FIRST_AVAILABLE = (select_first_placement_enum_func,)
    LAST_AVAILABLE = (select_last_placement_enum_func,)
    SHORTEST = (select_shortest_placement_enum_func,)
    LONGEST = (select_longest_placement_enum_func,)

    def __call__(self, *args, **kwargs):
        return self.value[0](*args, **kwargs)


# ----------- Find all node placement permutations with no constraints ---
def place_nodes_recursively_no_placement_check(available_nodes, past_nodes, all_nodes, current_run, current_plan,
                                               all_plans):
    if available_nodes:
        for node in available_nodes:
            # Update available nodes copy for next recursive calls
            new_available_nodes = available_nodes.copy()
            new_available_nodes.remove(node)
            new_past_nodes = past_nodes.copy()
            new_past_nodes.append(node)
            new_available_nodes.extend(
                get_new_available_nodes(node, new_past_nodes, all_nodes))

            # Place node in current run
            new_current_run = current_run.copy()
            new_current_run.append(node)
            place_nodes_recursively_no_placement_check(new_available_nodes.copy(), new_past_nodes, all_nodes,
                                                       new_current_run,
                                                       current_plan.copy(), all_plans)

            # Schedule current run and place node in next run
            new_current_plan = current_plan.copy()
            if current_run:
                new_current_plan.append(tuple(current_run))
            place_nodes_recursively_no_placement_check(new_available_nodes.copy(), new_past_nodes, all_nodes, [node],
                                                       new_current_plan,
                                                       all_plans)
    else:
        # Out of nodes -> Save current plan and return
        current_plan.append(tuple(current_run))
        all_plans.append(tuple(current_plan))
        return


# Util function to find new available nodes which have all of their prereq
# nodes in the past nodes list
def get_new_available_nodes(scheduled_node, past_nodes, all_nodes):
    potential_nodes = set(all_nodes[scheduled_node]["after"])
    new_available_nodes = potential_nodes.copy()
    for potential_node in potential_nodes:
        if potential_node != "":
            for previous_node, stream_index in all_nodes[potential_node]["before"]:
                if stream_index != -1 and previous_node not in past_nodes:
                    new_available_nodes.discard(potential_node)

    processed_new_available_nodes = [
        node for node in new_available_nodes if node != ""]
    return processed_new_available_nodes


def get_new_streamed_data_size(current_run, node_name, data_tables, all_nodes):
    before_node_names = [stream[0]
                         for stream in all_nodes[node_name]["before"]]
    required_tables = all_nodes[node_name]["tables"].copy()
    for placement in current_run:
        if placement.node_name in before_node_names:
            required_tables[before_node_names.index(placement.node_name)] = ""

    streamed_data_size = 0
    for table_name in required_tables:
        if table_name:
            streamed_data_size += data_tables[table_name].get(
                "record_size", 14) * data_tables[table_name]["record_count"]
    return streamed_data_size


# ----------- Main recursive loop -----------
def place_nodes_recursively(available_nodes, past_nodes, all_nodes, current_run, current_plan,
                            all_plans, reduce_single_runs, hw_library, current_min_runs_ptr,
                            data_tables, module_placement_selections, skipped_placements_stat_ptr, first_nodes,
                            blocked_nodes, next_run_blocked, current_start_time, trigger_timeout, time_limit, use_max_runs_cap, streamed_data_size):
    if trigger_timeout[0]:
        raise RuntimeError(
            f"Timeout: {perf_counter() - current_start_time:.3f}s")
    if not (len(current_plan) >
            current_min_runs_ptr[0] and use_max_runs_cap) and not trigger_timeout[0]:
        if available_nodes and not available_nodes.issubset(blocked_nodes):
            available_nodes_in_this_run = remove_unavailable_nodes_in_this_run(
                available_nodes, current_run, hw_library, all_nodes, first_nodes, blocked_nodes)
            available_module_placements = []
            for node in available_nodes:
                if node in available_nodes_in_this_run:
                    min_position = get_min_position_in_current_run(
                        current_run, node, all_nodes)
                    taken_columns = get_taken_columns(current_run)
                    available_module_placements.extend(get_scheduled_modules_for_node_after_pos(all_nodes, min_position,
                                                                                                node,
                                                                                                taken_columns, hw_library,
                                                                                                module_placement_selections,
                                                                                                skipped_placements_stat_ptr))

            if available_module_placements:
                # Place node in current run
                for insert_at, module_placement in available_module_placements:
                    # if trigger_timeout[0]:
                    #     return
                    new_current_run = current_run.copy()
                    new_current_run[insert_at:insert_at] = [
                        module_placement]
                    streamed_data_size += get_new_streamed_data_size(
                        current_run, module_placement.node_name, data_tables, all_nodes)
                    find_next_module_placement(all_nodes, all_plans, available_nodes, current_plan, hw_library,
                                               module_placement, new_current_run, module_placement.node_name, past_nodes,
                                               reduce_single_runs, current_min_runs_ptr, data_tables,
                                               module_placement_selections, skipped_placements_stat_ptr,
                                               first_nodes, blocked_nodes, next_run_blocked, current_start_time,
                                               trigger_timeout, time_limit, use_max_runs_cap, streamed_data_size)
            if (not available_module_placements or not reduce_single_runs) and current_run:
                for node in available_nodes:
                    if node not in blocked_nodes:
                        # Check if current run contains reducing module. If so
                        # then add the node to blocked nodes and do nothing.
                        # (for now - needs further testing)
                        new_next_run_blocked = set()
                        new_blocked_nodes = blocked_nodes.copy()
                        # Comment this out for no blocking
                        new_blocked_nodes.update(next_run_blocked)
                        if node in new_blocked_nodes:
                            place_nodes_recursively(available_nodes, past_nodes, all_nodes, current_run, current_plan,
                                                    all_plans, reduce_single_runs, hw_library, current_min_runs_ptr,
                                                    data_tables, module_placement_selections, skipped_placements_stat_ptr,
                                                    first_nodes, new_blocked_nodes, new_next_run_blocked, current_start_time,
                                                    trigger_timeout, time_limit, use_max_runs_cap, streamed_data_size)
                            return

                        # Schedule current run and place node in next run
                        new_current_plan = current_plan.copy()
                        if current_run:
                            new_current_plan.append(tuple(current_run))
                        available_module_placements = get_scheduled_modules_for_node_after_pos(all_nodes, 0, node, [],
                                                                                               hw_library,
                                                                                               module_placement_selections,
                                                                                               skipped_placements_stat_ptr)
                        if available_module_placements:
                            for _, module_placement in available_module_placements:
                                streamed_data_size += get_new_streamed_data_size(
                                    current_run, node, data_tables, all_nodes)
                                # if trigger_timeout[0]:
                                #     return
                                find_next_module_placement(all_nodes, all_plans, available_nodes, new_current_plan,
                                                           hw_library,
                                                           module_placement, [
                                                               module_placement], node, past_nodes,
                                                           reduce_single_runs, current_min_runs_ptr, data_tables,
                                                           module_placement_selections, skipped_placements_stat_ptr,
                                                           first_nodes, new_blocked_nodes, new_next_run_blocked, current_start_time,
                                                           trigger_timeout, time_limit, use_max_runs_cap, streamed_data_size)
                        else:
                            raise ValueError(
                                "Something went wrong - Should be able to place nodes in an empty run!")

        else:
            # Out of nodes -> Save current plan and return
            current_plan.append(tuple(current_run))
            if tuple(current_plan) not in all_plans:
                all_plans[tuple(current_plan)] = {
                    "next_nodes": available_nodes.copy(), "graph": all_nodes.copy(), "tables": data_tables.copy(), "past_nodes": past_nodes.copy(), "streamed_data_size": streamed_data_size}
                if len(current_plan) < current_min_runs_ptr[0]:
                    current_min_runs_ptr[0] = len(current_plan)
            if (perf_counter() - current_start_time > time_limit):
                trigger_timeout[0] = True
    elif not trigger_timeout[0]:
        if (perf_counter() - current_start_time > time_limit):
            trigger_timeout[0] = True


# ----------- Find nodes available for current run -----------
def remove_unavailable_nodes_in_this_run(available_nodes, current_run, hw_library, all_nodes, first_nodes,
                                         blocked_nodes):
    available_nodes_for_this_run = available_nodes.copy()
    for node in available_nodes:
        if node in first_nodes:
            for module in current_run:
                for before_node_name, stream_index in all_nodes[node]["before"]:
                    if stream_index != -1 and module.node_name == before_node_name:
                        available_nodes_for_this_run.discard(node)
        operation = all_nodes[node]["operation"]
        if "first_module" in hw_library[operation]["decorators"] \
                and current_run_has_first_module(hw_library, current_run,
                                                 node):
            available_nodes_for_this_run.discard(node)
        if node in blocked_nodes:
            available_nodes_for_this_run.discard(node)
    return available_nodes_for_this_run


def current_run_has_first_module(hw_library, current_run, node):
    for scheduled_module in current_run:
        if scheduled_module.node_name != node \
                and "first_module" in hw_library[scheduled_module.operation]["decorators"]:
            return True
    return False


# ----------- Helper methods for finding constraints for placing nodes in
def get_min_position_in_current_run(current_run, node, all_nodes):
    prereq_nodes = all_nodes[node]["before"]
    currently_scheduled_prereq_nodes = []
    for previous_node, _ in prereq_nodes:
        for module in current_run:
            if previous_node == module.node_name:
                currently_scheduled_prereq_nodes.append(module)
    if currently_scheduled_prereq_nodes:
        current_min = 0
        for module in currently_scheduled_prereq_nodes:
            if module.position[1] > current_min:
                current_min = module.position[1]
        return current_min + 1
    else:
        return 0


def get_taken_columns(current_run):
    taken_columns = []
    for module in current_run:
        taken_columns.append(module.position)
    return taken_columns


# ----------- Get possible bitstream placings -----------
def get_scheduled_modules_for_node_after_pos(all_nodes, min_position, node, taken_columns, hw_library,
                                             module_placement_selections, skipped_placements):
    current_operation = all_nodes[node]["operation"]
    available_module_placements = []
    if all_nodes[node]["satisfying_bitstreams"] and len(
            module_placement_selections) != 1:
        available_module_placements = get_chosen_module_placements(
            node, hw_library, skipped_placements, current_operation, module_placement_selections[
                1], min_position,
            taken_columns, all_nodes[node]["satisfying_bitstreams"])
    if not available_module_placements:
        available_module_placements = get_chosen_module_placements(
            node, hw_library, skipped_placements, current_operation, module_placement_selections[
                0], min_position,
            taken_columns, hw_library[current_operation]["start_locations"])
    return available_module_placements


def get_chosen_module_placements(node, hw_library, skipped_placements, current_operation, placement_selections,
                                 min_position, taken_columns, start_locations):
    available_bitstreams = find_all_available_bitstreams_after_min_pos(
        current_operation, min_position, taken_columns, hw_library, start_locations)
    available_module_placements = []
    if available_bitstreams:
        for chosen_module_position, chosen_column_position, chosen_bitstream_index in available_bitstreams:
            chosen_bitstream, end_index = get_bitstream_end_from_library_with_operation(chosen_bitstream_index,
                                                                                        chosen_column_position,
                                                                                        current_operation, hw_library)
            available_module_placements.append(
                (chosen_module_position,
                 ScheduledModule(node, current_operation, chosen_bitstream, (chosen_column_position, end_index))))
        # For stats gathering: 0 - alternatives; 1 - nodes placed
        skipped_placements[0] += len(available_module_placements)
        available_module_placements = select_according_to_preferences(available_module_placements,
                                                                      placement_selections)
        skipped_placements[0] -= len(available_module_placements)
        skipped_placements[1] += 1
    return available_module_placements


def find_all_available_bitstreams_after_min_pos(
        operation, min_position, taken_columns, hw_library, start_locations):
    all_positions_and_bitstreams = []
    for start_location_index in range(len(start_locations))[min_position:]:
        # This line possibly not needed
        if start_locations[start_location_index]:
            for bitstream in start_locations[start_location_index]:
                bitstream_index = hw_library[operation]["start_locations"][start_location_index].index(
                    bitstream)
                if not taken_columns:
                    all_positions_and_bitstreams.append(
                        (0, start_location_index, bitstream_index))
                else:
                    # Here need to check if beginning is after last taken and
                    # end is before next taken.
                    module_index = get_module_index(
                        start_location_index, taken_columns)
                    end_index = start_location_index + hw_library[operation]["bitstreams"][bitstream][
                        "length"] - 1
                    # If fits then append
                    if (len(taken_columns) == module_index and taken_columns[module_index - 1][
                        1] < start_location_index) or (
                            len(taken_columns) != module_index and taken_columns[module_index][
                                0] > end_index):
                        all_positions_and_bitstreams.append(
                            (module_index, start_location_index, bitstream_index))
    return all_positions_and_bitstreams


def get_bitstream_end_from_library_with_operation(chosen_bitstream_index, chosen_column_position, current_operation,
                                                  hw_library):
    chosen_bitstream = hw_library[current_operation]["start_locations"][chosen_column_position][
        chosen_bitstream_index]
    end_index = chosen_column_position + hw_library[current_operation]["bitstreams"][chosen_bitstream][
        "length"] - 1
    return chosen_bitstream, end_index


def get_module_index(start_location_index, taken_columns):
    if not taken_columns:
        raise ValueError("Taken positions can't be empty!")
    if start_location_index < taken_columns[0][0]:
        return 0
    for module_index in list(range(len(taken_columns)))[::-1]:
        if start_location_index > taken_columns[module_index][1]:
            return module_index + 1
    return len(taken_columns)


def select_according_to_preferences(
        available_module_placements, module_placement_selections):
    all_selected_placements = []  # For ORing together at the end
    for module_placement_clause in module_placement_selections:
        current_selected_placements = available_module_placements.copy()  # For ANDing together
        for placement_selection_function in module_placement_clause:
            current_selected_placements = placement_selection_function(
                current_selected_placements)
        all_selected_placements.append(current_selected_placements)
    chosen_module_placements = []
    for chosen_module_set in all_selected_placements:
        for chosen_module in chosen_module_set:
            if chosen_module not in chosen_module_placements:
                chosen_module_placements.append(chosen_module)
    return chosen_module_placements


# ----------- Update meta data according to the chosen bitstream and proce
def find_next_module_placement(all_nodes, all_plans, available_nodes, new_current_plan, hw_library, module_placement,
                               new_current_run, node, past_nodes, reduce_single_runs, current_min_runs, data_tables,
                               module_placement_selections, skipped_placements, first_nodes, blocked_nodes,
                               next_run_blocked, current_start_time, trigger_timeout, time_limit, use_max_runs_cap, streamed_data_size):
    # Check requirements and utility and update all nodes accordingly no
    # matter if the node is removed or not.
    new_all_nodes, new_data_tables, satisfies_requirements, skipped_nodes = update_all_nodes(all_nodes,
                                                                                             module_placement.bitstream,
                                                                                             data_tables, hw_library,
                                                                                             node,
                                                                                             all_nodes[node][
                                                                                                 "capacity"],
                                                                                             all_nodes[node][
                                                                                                 "operation"])
    # Set new available and past nodes if the current node can be removed
    new_available_nodes, new_past_nodes = create_new_available_nodes_lists(all_nodes,
                                                                           available_nodes,
                                                                           past_nodes, node,
                                                                           satisfies_requirements)
    for skipped_node in skipped_nodes:
        if skipped_node not in new_available_nodes:
            raise ValueError("Skipped nodes processes in the wrong order!")
        else:
            new_available_nodes, new_past_nodes = create_new_available_nodes_lists(all_nodes,
                                                                                   new_available_nodes,
                                                                                   new_past_nodes, skipped_node,
                                                                                   satisfies_requirements)

    # Check new graph and old graph and if all of the nodes in the new graph
    # are the same as the old graph then it's fine. Otherwise redo the
    # new_graph!
    update_satisfying_bitstreams(
        node, all_nodes, new_all_nodes, new_available_nodes, hw_library, data_tables, new_data_tables, new_past_nodes)

    # Update next_run_blocked
    new_next_run_blocked = get_new_blocked_nodes(
        next_run_blocked, hw_library, module_placement, all_nodes)

    # Next recursive step
    place_nodes_recursively(new_available_nodes, new_past_nodes,
                            new_all_nodes, new_current_run.copy(), new_current_plan.copy(),
                            all_plans, reduce_single_runs, hw_library, current_min_runs,
                            new_data_tables, module_placement_selections, skipped_placements, first_nodes,
                            blocked_nodes, new_next_run_blocked, current_start_time, trigger_timeout, time_limit, use_max_runs_cap, streamed_data_size)


# Update node availability based on current selection satisfiability
def create_new_available_nodes_lists(
        all_nodes, available_nodes, past_nodes, node, satisfies_requirements):
    new_available_nodes = available_nodes.copy()
    new_past_nodes = past_nodes.copy()
    if satisfies_requirements:
        new_available_nodes.remove(node)
        new_past_nodes.add(node)
        new_available_nodes.update(
            get_new_available_nodes(node, new_past_nodes, all_nodes))
    return new_available_nodes, new_past_nodes


def update_satisfying_bitstreams(current_node, previous_all_nodes, new_all_nodes, new_available_nodes, hw_library,
                                 old_data_tables, new_data_tables, new_past_nodes):
    update_required = False
    for node_name in new_all_nodes.keys():
        if previous_all_nodes[node_name]["capacity"] != new_all_nodes[node_name]["capacity"]:
            update_required = True
            break
        update_required = check_table_equality_of_given_node(
            previous_all_nodes, new_all_nodes, node_name, old_data_tables, new_data_tables)
        if update_required:
            break
    if not update_required:
        for next_node in previous_all_nodes[current_node]["after"]:
            if next_node != "":
                if next_node not in new_all_nodes:
                    update_required = True
                else:
                    update_required = check_table_equality_of_given_node(
                        previous_all_nodes, new_all_nodes, next_node, old_data_tables, new_data_tables)
                if update_required:
                    break
    if update_required:
        current_tables = []
        for node_check in previous_all_nodes.keys():
            current_tables.append(
                previous_all_nodes[node_check]["tables"].copy())
        add_satisfying_bitstream_locations_to_graph(
            new_available_nodes, new_all_nodes, hw_library, new_data_tables, new_past_nodes)
        new_tables = []
        for node_check in previous_all_nodes.keys():
            new_tables.append(previous_all_nodes[node_check]["tables"].copy())
        for table_i in range(len(new_tables)):
            if new_tables[table_i] != current_tables[table_i]:
                raise ValueError("Something went wrong!")


def check_table_equality_of_given_node(
        previous_all_nodes, new_all_nodes, node_name, old_data_tables, new_data_tables):
    for table_index in range(len(previous_all_nodes[node_name]["tables"])):
        # old_table_name = previous_all_nodes[node_name]["tables"][table_index]
        # new_table_name = new_all_nodes[node_name]["tables"][table_index]
        if old_data_tables[previous_all_nodes[node_name]["tables"][table_index]] != new_data_tables[
                new_all_nodes[node_name]["tables"][table_index]]:
            return True
    return False


def find_data_sensitive_node_names(node_name, all_nodes, new_next_run_blocked):
    for next_node_name in all_nodes[node_name]["after"]:
        if next_node_name != "":
            if all_nodes[next_node_name]["operation"] == "Merge Sort":
                new_next_run_blocked.add(next_node_name)
            find_data_sensitive_node_names(
                next_node_name, all_nodes, new_next_run_blocked)


def get_new_blocked_nodes(next_run_blocked, hw_library,
                          module_placement, all_nodes):
    new_next_run_blocked = next_run_blocked.copy()
    if "reducing" in hw_library[module_placement.operation]["decorators"]:
        find_data_sensitive_node_names(
            module_placement.node_name, all_nodes, new_next_run_blocked)
    # if module_placement.node_name not in next_run_blocked:
    #     if "reducing" in hw_library[module_placement.operation]["decorators"]:
    #         # Don't add current node
    #         # Check if next node
    #         new_next_run_blocked.add(module_placement.node_name)
    #         for next_node_name in all_nodes[module_placement.node_name]["after"]:
    #             if next_node_name != "" and next_node_name not in next_run_blocked:
    #                 new_next_run_blocked.add(next_node_name)
    # else:
    #     for next_node_name in all_nodes[module_placement.node_name]["after"]:
    #         if next_node_name != "" and next_node_name not in next_run_blocked:
    #             new_next_run_blocked.add(next_node_name)
    return new_next_run_blocked


# -------- Find out if current selection satisfies operation requirements
def update_all_nodes(all_nodes, bitstream, data_tables,
                     hw_library, node, node_cost, operation):
    # available_nodes, graph, hw_library, data_tables
    bitstream_capacity = hw_library[operation]["bitstreams"][bitstream]["capacity"]
    missing_utility = []
    new_all_nodes = all_nodes.copy()
    for new_node in new_all_nodes.keys():
        new_all_nodes[new_node] = all_nodes[new_node].copy()
        new_all_nodes[new_node]["tables"] = all_nodes[new_node]["tables"].copy()
    new_data_tables = data_tables.copy()
    skipped_nodes = []
    if "sorting" in hw_library[operation]["decorators"]:
        # Update table
        should_node_be_removed = update_node_data_tables(all_nodes, node, bitstream_capacity,
                                                         hw_library[operation]["decorators"],
                                                         data_tables, new_data_tables)
        skipped_nodes = check_for_skippable_sort_operations(
            new_all_nodes, new_data_tables, node, hw_library)
    else:
        # Find missing utility
        should_node_be_removed = find_missing_utility(
            bitstream_capacity, missing_utility, node_cost)
        update_graph_capacities(
            all_nodes, missing_utility, new_all_nodes, node, should_node_be_removed)
    # Update all nodes
    if should_node_be_removed:
        resulting_tables = get_resulting_tables(all_nodes[node]["tables"],
                                                hw_library[operation]["decorators"],
                                                new_data_tables)
        update_next_node_tables(
            all_nodes, node, new_all_nodes, skipped_nodes, resulting_tables)
    return new_all_nodes, new_data_tables, should_node_be_removed, skipped_nodes


# ==Sorting==
# This would be done by the operation driver
def update_node_data_tables(all_nodes, node, bitstream_capacity,
                            current_node_decorators, data_tables, new_data_tables):
    if "partial_sort" in current_node_decorators:
        if len(all_nodes[node]["tables"]) != 1:
            raise ValueError("Wrong number of tables for linear sort!")
        if len(bitstream_capacity) != 1:
            raise ValueError("Wrong linear sort capacity given!")
        table_name = all_nodes[node]["tables"][0]
        current_table = data_tables[table_name].copy()
        new_sorted_sequences = get_linear_sorter_sequences(
            bitstream_capacity[0], current_table["record_count"])
        current_table["sorted_sequences"] = tuple(new_sorted_sequences)
        new_data_tables[table_name] = current_table
        return True
    elif "blocking_sort" in current_node_decorators:
        if len(all_nodes[node]["tables"]) != 1:
            raise ValueError("Wrong number of tables for merge sort!")
        if len(bitstream_capacity) != 1:
            raise ValueError("Wrong merge sort capacity given!")
        table_name = all_nodes[node]["tables"][0]
        current_table = data_tables[table_name].copy()
        # Not checking if it is sorted with the correct column_id (just id 0)
        if is_table_sorted(current_table, 0):
            raise ValueError("Table is sorted already!")
        new_sorted_sequences = []
        current_sequences = current_table["sorted_sequences"]
        if len(current_sequences) == 0:
            new_sorted_sequences.append(
                (0, min(bitstream_capacity[0], current_table["record_count"]), 0))
        else:
            new_sequence_length = 0
            for i in range(min(bitstream_capacity[0], len(current_sequences))):
                new_sequence_length += current_sequences[i][1]
            if bitstream_capacity[0] > len(
                    current_sequences) and new_sequence_length < current_table["record_count"]:
                new_sequence_length += bitstream_capacity[0] - len(
                    current_sequences)
                new_sequence_length = min(
                    new_sequence_length, current_table["record_count"])
            new_sorted_sequences.append((0, new_sequence_length, 0))
            if new_sequence_length < current_table["record_count"] and bitstream_capacity[0] < len(
                    current_sequences):
                for sequence_i in range(
                        bitstream_capacity[0], len(current_sequences)):
                    new_sorted_sequences.append(current_sequences[sequence_i])
        current_table["sorted_sequences"] = tuple(new_sorted_sequences)
        new_data_tables[table_name] = current_table
        return is_table_sorted(current_table, 0)


def get_linear_sorter_sequences(bitstream_capacity, record_count):
    new_sorted_sequences = []
    sequence_count = record_count // bitstream_capacity
    if sequence_count == 0:
        new_sorted_sequences.append((0, record_count, 0))
    else:
        for sequence_id in range(sequence_count):
            new_sorted_sequences.append(
                (bitstream_capacity * sequence_id, bitstream_capacity, 0))
            if sequence_id == sequence_count - 1:
                new_sorted_sequences.append(
                    (bitstream_capacity * sequence_count, record_count % bitstream_capacity, 0))
    return new_sorted_sequences


def is_table_sorted(table, column_id):
    return len(table["sorted_sequences"]) == 1 and table["sorted_sequences"][0][0] == 0 and \
        table["sorted_sequences"][0][1] == table["record_count"] and table["sorted_sequences"][0][2] == column_id


def check_for_skippable_sort_operations(
        new_all_nodes, new_data_tables, node, hw_library):
    all_tables_sorted = True
    for table_name in new_all_nodes[node]["tables"]:
        if not is_table_sorted(new_data_tables[table_name], 0):
            all_tables_sorted = False
            break
    skipped_nodes = []
    # TODO: For now just remove the next sorting operation - Not entirely
    # correct for the final product.
    if all_tables_sorted:
        for current_next_node_name in new_all_nodes[node]["after"]:
            if current_next_node_name != "" and "sorting" in hw_library[
                    new_all_nodes[current_next_node_name]["operation"]]["decorators"]:
                skipped_nodes.append(current_next_node_name)
    return skipped_nodes


# ==Other operations==
def find_missing_utility(bitstream_capacity, missing_utility, node_cost):
    should_node_be_removed = True
    if len(bitstream_capacity) != len(node_cost):
        raise ValueError("Capacity parameters don't match")
    for capacity_parameter_index in range(len(bitstream_capacity)):
        missing_utility.append(
            node_cost[capacity_parameter_index] - bitstream_capacity[capacity_parameter_index])
        if bitstream_capacity[capacity_parameter_index] < node_cost[capacity_parameter_index]:
            should_node_be_removed = False
    return should_node_be_removed


def update_graph_capacities(
        all_nodes, missing_utility, new_all_nodes, node, should_node_be_removed):
    if not should_node_be_removed:
        new_all_nodes[node] = all_nodes[node].copy()
        new_capacity_values = []
        for capacity_parameter_index in range(len(missing_utility)):
            if missing_utility[capacity_parameter_index] <= 0:
                new_capacity_values.append(0)
            else:
                new_capacity_values.append(
                    missing_utility[capacity_parameter_index])
        new_all_nodes[node]["capacity"] = tuple(new_capacity_values)


# ==Update Graph==
def get_resulting_tables(
        input_tables, current_node_decorators, new_data_tables):
    if len(input_tables) == 0:
        # Data generator decorators haven't been added yet!
        raise ValueError("No input data found!")
    if "sorted_input" in current_node_decorators:
        for table_name in input_tables:
            if not is_table_sorted(new_data_tables[table_name], 0):
                raise ValueError("Table should be sorted!")
    # Simplified decorator for equi-join
    if "largest_input_is_output" in current_node_decorators:
        max_table_name = ""
        max_size = 0
        for table_name in input_tables:
            if new_data_tables[table_name]["record_count"] >= max_size:
                max_table_name = table_name
                max_size = new_data_tables[table_name]["record_count"]
        return [max_table_name]
    # Other modules have input tables forwarded to output
    else:
        return input_tables


def update_next_node_tables(
        all_nodes, node, new_all_nodes, skipped_nodes, resulting_tables):
    add_new_table_to_next_nodes(
        all_nodes, new_all_nodes, node, resulting_tables)
    del new_all_nodes[node]
    # Assuming postorder tree structure of skipped_nodes
    # TODO: Check postorder
    for skipped_node in skipped_nodes:
        add_new_table_to_next_nodes(
            all_nodes, new_all_nodes, skipped_node, resulting_tables)
        del new_all_nodes[skipped_node]


def add_new_table_to_next_nodes(all_nodes, new_all_nodes, node, table_names):
    # for next_node in new_all_nodes[node]["after"]:
    #     if (next_node != ""):
    #         new_all_nodes[next_node] = all_nodes[next_node].copy()
    #         new_all_nodes[next_node]["tables"] = all_nodes[next_node]["tables"].copy()

    add_new_table_to_next_nodes_in_place(new_all_nodes, node, table_names)


def get_current_node_index(new_all_nodes, next_node, node_name):
    current_node_indexes = []
    for potential_current_node_i in range(
            len(new_all_nodes[next_node]["before"])):
        if new_all_nodes[next_node]["before"][potential_current_node_i][0] == node_name:
            current_node_indexes.append(
                (potential_current_node_i, new_all_nodes[next_node]["before"][potential_current_node_i][1]))
    if len(current_node_indexes) == 0:
        raise ValueError("No next nodes found with the expected dependencies!")
    return current_node_indexes


def find_reducing_nodes(graph, hw_library):
    reducing_nodes = set()
    for node_name in graph.keys():
        operation = graph[node_name]["operation"]
        if "reducing" in hw_library[operation]["decorators"]:
            reducing_nodes.add(node_name)
    return reducing_nodes


def reduce_table_sizes(node_name, starting_nodes,
                       original_graph, current_graph, data_tables, selectivity):
    if node_name in starting_nodes:
        # TODO: For simplicity this reduces all tables but the correct table
        # should be found out instead!
        for table in current_graph[node_name]["tables"]:
            if table != "":
                data_tables[table]["record_count"] = int(
                    data_tables[table]["record_count"] * selectivity)
                # print(f'Reduced:{table} from {node_name}')
                new_sequences = []
                for sequence in data_tables[table]["sorted_sequences"]:
                    if sequence[0] <= data_tables[table]["record_count"]:
                        new_sequence = list(sequence)
                        if sequence[0] + \
                                sequence[1] > data_tables[table]["record_count"]:
                            new_sequence[1] = data_tables[table]["record_count"] - sequence[0]
                        new_sequences.append(tuple(new_sequence))
                data_tables[table]["sorted_sequences"] = tuple(new_sequences)

    # Potentially else needed here!
    for next_node in original_graph[node_name]["after"]:
        if next_node != "":
            reduce_table_sizes(next_node, starting_nodes, original_graph,
                               current_graph, data_tables, selectivity)


# ----------- Main scheduling function to find the plans and statistics --
def find_plans_and_print(starting_nodes, graph, resource_string, hw_library, data_tables, saved_nodes,
                         module_placement_selections, selectivity, time_limit, utilites_scaler, config_written_scaler,
                         utility_per_frame_scaler, disallow_empty_runs, use_max_runs_cap, resource_column_costs, config_speed,
                         streaming_speed, calculate_exact, fail_on_timeout):
    # print_node_placement_permutations(starting_nodes, graph)
    # print(f"{FancyText.UNDERLINE}Plans with placed modules below:{FancyText.END}")
    # Now with string match checks

    past_nodes = set()
    current_run = []
    current_plan = []

    # overall_start_time = perf_counter()

    # Prepare first_nodes
    first_nodes = get_first_nodes_from_saved_nodes(saved_nodes, graph)
    add_all_first_modules_nodes_to_list(first_nodes, graph, hw_library)
    # Currently not possible to find any with valid graphs
    add_all_splitting_modules_nodes_to_list(first_nodes, graph)

    reducing_nodes = find_reducing_nodes(graph, hw_library)
    original_graph = copy.deepcopy(graph)

    # Change this to list later
    plan_count = 0
    placed_nodes = 0
    run_count = 0
    discarded_placements = 0
    plans_chosen = 0
    timeouts = 0
    overall_utility = 0
    overall_config_written = 0
    overall_score = 0.0
    overall_cost_evaluation_time = 0.0
    overall_runs = 0
    overall_exec_time = 0
    overall_config_time = 0
    overall_performance_time = 0

    current_configuration = []

    # Put this into a while (starting_nodes)
    while starting_nodes:
        resulting_plans = dict()
        min_runs_pointer = [maxsize]
        skipped_placements_stat_pointer = [0, 0]
        blocked_nodes = set()
        next_run_blocked_nodes = set()
        trigger_timeout = [False]

        # Find satisfying bitstreams
        add_satisfying_bitstream_locations_to_graph(
            starting_nodes, graph, hw_library, data_tables, past_nodes)
        # Start the main recursive method
        current_start_time = perf_counter()
        overall_start_time = perf_counter()
        try:
            place_nodes_recursively(starting_nodes, past_nodes, graph, current_run, current_plan, resulting_plans, disallow_empty_runs,
                                    hw_library, min_runs_pointer, data_tables, module_placement_selections,
                                    skipped_placements_stat_pointer, first_nodes, blocked_nodes, next_run_blocked_nodes, current_start_time, trigger_timeout, time_limit, use_max_runs_cap, 0)
        except RuntimeError as e:
            if fail_on_timeout:
                raise e

        # print(f"Number of full plans generated: {len(resulting_plans)}")
        # print(
        #     f"Number of nodes placed to runs: {skipped_placements_stat_pointer[1]}")
        # print(
        #     f"Number of module placements discarded: {skipped_placements_stat_pointer[0]}")
        # print(
        #     f"Number of discarded placements per node: {skipped_placements_stat_pointer[0] / skipped_placements_stat_pointer[1]:.3f}")
        # print_statistics(resulting_plans)
        # print_all_runs_using_less_runs_than(
        #     resulting_plans, min_runs_pointer[0] + 1, resource_string)

        # print(f"Time: {perf_counter()-current_start_time:.3f}s")

        overall_stop_time = perf_counter()
        overall_performance_time += overall_stop_time - overall_start_time

        plan_count += len(resulting_plans)
        placed_nodes += skipped_placements_stat_pointer[1]
        discarded_placements += skipped_placements_stat_pointer[0]
        plans_chosen += 1
        run_count += min_runs_pointer[0]

        if (trigger_timeout[0]):
            timeouts += 1

        best_plan, cost_evaluation_time = choose_best_plan(
            list(resulting_plans.keys()), min_runs_pointer[0],
            current_configuration, resource_string, utilites_scaler,
            config_written_scaler, utility_per_frame_scaler, resulting_plans, resource_column_costs,
            streaming_speed, config_speed, calculate_exact)

        overall_cost_evaluation_time += cost_evaluation_time
        #
        if calculate_exact:
            chosen_plan, exec_time, config_time = best_plan

            overall_exec_time += exec_time
            overall_config_time += config_time
        else:
            chosen_plan, chosen_utility, chosen_config_written, score = best_plan

            overall_config_written += chosen_config_written
            overall_score += score

        # for run in chosen_plan:
        #     bitstreams = []
        #     locations = []
        #     for module in run:
        #         bitstreams.append(module.bitstream)
        #         locations.append(module.position)
        #     print(bitstreams)
        # print(locations)
        # print(runtime)

        # print(f"Plan count: {len(resulting_plans)}")
        # print(f"Cost eval: {cost_evaluation_time}")
        # print(
        # f"Util - {chosen_utility}, Frames - {chosen_config_written}, Score -
        # {score}")

        current_configuration = resulting_plans[chosen_plan]["last_configuration"]
        overall_runs += len(chosen_plan)

        past_nodes = resulting_plans[chosen_plan]["past_nodes"]
        starting_nodes = resulting_plans[chosen_plan]["next_nodes"]
        graph = resulting_plans[chosen_plan]["graph"]
        data_tables = resulting_plans[chosen_plan]["tables"]

        for run in chosen_plan:
            for placed_module in run:
                if placed_module.node_name in reducing_nodes:
                    reducing_nodes.remove(placed_module.node_name)
                    reduce_table_sizes(
                        placed_module.node_name, starting_nodes, original_graph, graph, data_tables, selectivity)

    # overall_stop_time = perf_counter()
    # print(
    #     f"Scheduling duration: {overall_stop_time - overall_start_time - overall_cost_evaluation_time:.3f}s; Cost evaluation duration:{overall_cost_evaluation_time:.3f}s; Score: {(overall_score/plans_chosen) * 100:.2f}%")

    overall_utility = 1 / overall_runs

    if overall_config_written:
        utility_frame_ratio = overall_utility / overall_config_written
    else:
        utility_frame_ratio = -1

    # overall_performance_time = overall_stop_time - overall_start_time

    stats_list = [plan_count, placed_nodes, discarded_placements,
                  plans_chosen, run_count, overall_performance_time, overall_cost_evaluation_time,
                  timeouts, overall_utility, overall_config_written, utility_frame_ratio, (
                      overall_score / plans_chosen),
                  exec_time, config_time]
    converted_list = [str(element) for element in stats_list]
    return converted_list


# ----------- Preprocessing before scheduling util methods to find satisfy
# No node skipping as we don't know which bitstream will be picked.
def add_satisfying_bitstream_locations_to_graph(
        available_nodes, graph, hw_library, data_tables, given_processed_nodes):
    current_available_nodes = available_nodes.copy()
    processed_nodes = given_processed_nodes.copy()
    min_capacity = get_minimum_capacity_values(hw_library)

    while current_available_nodes:
        current_node_name = current_available_nodes.pop()
        processed_nodes.add(current_node_name)
        current_available_nodes.update(get_new_available_nodes(
            current_node_name, processed_nodes, graph))
        min_requirements = get_min_requirements(
            current_node_name, graph, hw_library, data_tables)
        list_of_fitting_bitstreams = find_adequate_bitstreams(min_requirements, graph[current_node_name]["operation"],
                                                              hw_library)
        if list_of_fitting_bitstreams:
            fitting_bitstream_locations = get_fitting_bitstream_locations_based_on_list(list_of_fitting_bitstreams,
                                                                                        hw_library[
                                                                                            graph[current_node_name][
                                                                                                "operation"]][
                                                                                            "start_locations"])
            graph[current_node_name]["satisfying_bitstreams"] = fitting_bitstream_locations
        resulting_tables = get_worst_case_fully_processed_tables(graph[current_node_name]["tables"],
                                                                 hw_library[graph[current_node_name]["operation"]][
                                                                     "decorators"], data_tables,
                                                                 min_capacity[graph[current_node_name]["operation"]])
        add_new_table_to_next_nodes_in_place(
            graph, current_node_name, resulting_tables)

        if len(min_requirements) == 1 and min_requirements[0] == 0:
            if current_node_name in available_nodes:
                available_nodes.remove(current_node_name)
                temp_past_nodes = processed_nodes.copy()
                temp_past_nodes.add(current_node_name)
                available_nodes.update(get_new_available_nodes(
                    current_node_name, temp_past_nodes, graph))
            remove_from_graph(graph, current_node_name)


def remove_from_graph(graph, current_node_name):
    # Before node check has to put in as well. Can't quite handle multiple inputs
    # And also check how many inputs or how many outputs.
    # Hardcoded for 1 in 1 out at the moment.

    # If it is not one in and one out - throw an error
    if (len(graph[current_node_name]["after"]) != 1 or len(
            graph[current_node_name]["before"]) != 1):
        raise ValueError("Not implemented")
    after_node = graph[current_node_name]["after"][0]
    before_node_data = graph[current_node_name]["before"][0]

    if after_node == "" and before_node_data == ("", -1):
        pass
    elif after_node == "":
        if before_node_data[0] in graph:
            new_after_nodes = list(graph[before_node_data[0]]["after"])
            new_after_nodes[before_node_data[1]] = after_node
            graph[before_node_data[0]]["after"] = tuple(new_after_nodes)
        # else:
        #     print("Warn: No previous node found!")
    elif before_node_data == ("", -1):
        index = get_current_node_index(
            graph, after_node, current_node_name)[0][0]
        new_before_nodes = list(graph[after_node]["before"])
        new_before_nodes[index] = before_node_data
        graph[after_node]["before"] = tuple(new_before_nodes)
    else:
        if before_node_data[0] in graph:
            new_after_nodes = list(graph[before_node_data[0]]["after"])
            new_after_nodes[before_node_data[1]] = after_node
            graph[before_node_data[0]]["after"] = tuple(new_after_nodes)
        # else:
        #     print("Warn: No previous node found!")
        index = get_current_node_index(
            graph, after_node, current_node_name)[0][0]
        new_before_nodes = list(graph[after_node]["before"])
        new_before_nodes[index] = before_node_data
        graph[after_node]["before"] = tuple(new_before_nodes)
    del graph[current_node_name]


def get_minimum_capacity_values(hw_library):
    min_capacity = {}
    for operation, operation_bitstream_parameters in hw_library.items():
        for _, bitstream_params in operation_bitstream_parameters["bitstreams"].items(
        ):
            if operation not in min_capacity:
                min_capacity[operation] = bitstream_params["capacity"]
            else:
                smaller_bitstream_found = True
                for capacity_parameter in range(
                        len(bitstream_params["capacity"])):
                    if bitstream_params["capacity"][capacity_parameter] > min_capacity[operation][capacity_parameter]:
                        smaller_bitstream_found = False
                        break
                if smaller_bitstream_found:
                    min_capacity[operation] = bitstream_params["capacity"]
    return min_capacity


def get_min_requirements(current_node_name, graph, hw_library, data_tables):
    node_operation = graph[current_node_name]["operation"]
    current_decorator_list = hw_library[node_operation]["decorators"]
    if "sorting" in current_decorator_list:
        tables_to_be_sorted = graph[current_node_name]["tables"]
        if len(tables_to_be_sorted) != 1:
            raise ValueError("Currently sorters only support one input table")
        if "partial_sort" in current_decorator_list:
            return (data_tables[tables_to_be_sorted[0]]["record_count"],)
        elif "blocking_sort" in current_decorator_list:
            # TODO: Add a check that correct things are being sorted
            current_table_sorted_sequences = data_tables[tables_to_be_sorted[0]
                                                         ]["sorted_sequences"]
            if not current_table_sorted_sequences:
                return (data_tables[tables_to_be_sorted[0]]["record_count"],)
            else:
                if is_table_sorted(data_tables[tables_to_be_sorted[0]], 0):
                    return (0,)
                pos_of_last_sorted_element = current_table_sorted_sequences[-1][0] + \
                    current_table_sorted_sequences[-1][1]
                if pos_of_last_sorted_element > data_tables[tables_to_be_sorted[0]
                                                            ]["record_count"]:
                    raise ValueError("Incorrect sorted sequences!")
                unsorted_tail_length = data_tables[tables_to_be_sorted[0]
                                                   ]["record_count"] - pos_of_last_sorted_element
                return (len(current_table_sorted_sequences) +
                        unsorted_tail_length,)
    else:
        return graph[current_node_name]["capacity"]


def find_adequate_bitstreams(min_requirements, operation, hw_library):
    fitting_bitstreams = set()
    for bitstream_name, bitstream_parameters in hw_library[operation]["bitstreams"].items(
    ):
        bitstream_utility_is_great_enough = True
        for capacity_parameter_i in range(
                len(bitstream_parameters["capacity"])):
            if bitstream_parameters["capacity"][capacity_parameter_i] < min_requirements[capacity_parameter_i]:
                bitstream_utility_is_great_enough = False
                break
        if bitstream_utility_is_great_enough:
            fitting_bitstreams.add(bitstream_name)
    return fitting_bitstreams


def get_fitting_bitstream_locations_based_on_list(
        list_of_fitting_bitstreams, start_locations):
    fitting_bitstream_locations = []
    for location_index in range(len(start_locations)):
        fitting_bitstream_locations.append(
            start_locations[location_index].copy())
        for bitstream_index in range(
                len(start_locations[location_index]))[::-1]:
            if start_locations[location_index][bitstream_index] not in list_of_fitting_bitstreams:
                del fitting_bitstream_locations[location_index][bitstream_index]
    return fitting_bitstream_locations


# ----------- Update tables in preprocessing ------------
def get_worst_case_fully_processed_tables(
        input_tables, current_node_decorators, data_tables, min_capacity):
    if len(input_tables) == 0:
        # Data generator decorators haven't been added yet!
        raise ValueError("No input data found!")
    elif "partial_sort" in current_node_decorators:
        if len(min_capacity) != 1:
            raise ValueError("Incorrect capacity values given!")
        half_sorted_tables = []
        for table_name in input_tables:
            new_table_name = table_name
            new_table_data = data_tables[table_name].copy()
            new_table_data["sorted_sequences"] = tuple(
                get_linear_sorter_sequences(min_capacity[0], new_table_data["record_count"]))
            if len(new_table_data["sorted_sequences"]) != 1:
                new_table_name += "_half_sorted"
            else:
                new_table_name += "_fully_sorted"
            half_sorted_tables.append(new_table_name)
            data_tables[new_table_name] = new_table_data
        return half_sorted_tables
    elif "blocking_sort" in current_node_decorators:
        sorted_tables = []
        for table_name in input_tables:
            if is_table_sorted(data_tables[table_name], 0):
                sorted_tables.append(table_name)
            else:
                new_table_name = table_name + "_fully_sorted"
                new_table_data = data_tables[table_name].copy()
                new_table_data["sorted_sequences"] = (
                    (0, new_table_data["record_count"], 0),)
                sorted_tables.append(new_table_name)
                data_tables[new_table_name] = new_table_data
        return sorted_tables
    # Simplified decorator for equi-join
    elif "largest_input_is_output" in current_node_decorators:
        max_table_name = ""
        max_size = 0
        for table_name in input_tables:
            if data_tables[table_name]["record_count"] >= max_size:
                max_table_name = table_name
                max_size = data_tables[table_name]["record_count"]
        return [max_table_name]
    # Other modules have input tables forwarded to output
    else:
        return input_tables


def add_new_table_to_next_nodes_in_place(all_nodes, node, table_names):
    for next_node in all_nodes[node]["after"]:
        if (next_node != ""):
            # Get index
            current_node_indexes = get_current_node_index(
                all_nodes, next_node, node)

            for node_table_index, current_node_stream_index in current_node_indexes:
                # Assuming it is large enough!
                all_nodes[next_node]["tables"][node_table_index] = table_names[current_node_stream_index]


# ----------- Preprocessing before scheduling util methods to find breakin
def get_first_nodes_from_saved_nodes(saved_nodes, graph):
    first_nodes = set()
    for saved_node in saved_nodes:
        for first_node in graph[saved_node]["after"]:
            if first_node != "":
                first_nodes.add(first_node)
    return first_nodes


def add_all_first_modules_nodes_to_list(first_nodes, graph, hw_library):
    for node_name, node_parameters in graph.items():
        if "first_module" in hw_library[node_parameters["operation"]
                                        ]["decorators"]:
            first_nodes.add(node_name)


def add_all_splitting_modules_nodes_to_list(first_nodes, graph):
    all_stream_dependencies = set()
    splitting_streams = set()
    for node_name in graph.keys():
        for previous_node in graph[node_name]["before"]:
            if previous_node[1] != -1:
                if previous_node not in all_stream_dependencies:
                    all_stream_dependencies.add(previous_node)
                else:
                    splitting_streams.add(previous_node)
    for splitting_node in splitting_streams:
        for node in graph[splitting_node[0]]["after"]:
            if node != "" and splitting_node in graph[node]["before"]:
                first_nodes.add(node)


# ----------- Print Scheduling results -----------
def print_statistics(plans):
    print(f"Unique plan count:{len(plans)}")
    run_counts = {}
    for plan in plans:
        run_count = len(plan)
        if run_count in run_counts:
            run_counts[run_count] += 1
        else:
            run_counts[run_count] = 1
    for run_count, occurrences in sorted(run_counts.items()):
        print(f"{run_count} run plans:{occurrences}")


def print_all_runs_using_less_runs_than(
        plans_dict, run_limit, resource_string):
    plans = list(plans_dict.keys())
    for plan_i in range(len(plans)):
        if len(plans[plan_i]) < run_limit:
            print()
            print(f"Plan {plan_i}:")
            for run_i in range(len(plans[plan_i])):
                print(f"Run#{run_i}:")
                node_names = []
                bitstream_names = []
                start_and_ends = []
                for module in plans[plan_i][run_i]:
                    node_names.append(module.node_name)
                    bitstream_names.append(module.bitstream)
                    start_and_ends.append(module.position)
                print(node_names)
                print(bitstream_names)
                print_resource_string(resource_string, start_and_ends)
            if plans_dict[plans[plan_i]]["next_nodes"]:
                print(
                    f"Next nodes to schedule: {plans_dict[plans[plan_i]]['next_nodes']}")


def print_resource_string(current_resource_string, module_coordinates):
    colour_char_len = len(FancyText.GREEN)
    for coordinate_i in range(len(module_coordinates)):
        coordinate = module_coordinates[coordinate_i]
        resulting_string = current_resource_string[:coordinate[0] + coordinate_i * 2 * colour_char_len] \
            + FancyText.GREEN + current_resource_string[
            coordinate[0] + coordinate_i * 2 * colour_char_len:]
        current_resource_string = resulting_string
        resulting_string = current_resource_string[:(coordinate[1] + 1) + (coordinate_i * 2 + 1) * colour_char_len] \
            + FancyText.DEFAULT + current_resource_string[
            (coordinate[1] + 1) + (coordinate_i * 2 + 1) * colour_char_len:]
        current_resource_string = resulting_string
    print(current_resource_string)


# ----------- Print plans with no constraints -----------
def print_node_placement_permutations(available_nodes, all_nodes):
    simple_plans = []
    # For each choice:
    # Make the choice and then update available nodes and come back to initial
    # step
    place_nodes_recursively_no_placement_check(
        available_nodes, [], all_nodes, [], [], simple_plans)
    # Remove duplicates
    simple_plans = list(dict.fromkeys(simple_plans))
    # Print plans
    for plan_i in range(len(simple_plans)):
        print(f"{plan_i}: {simple_plans[plan_i]}")


# --------- Choose best plan ----------------

def find_utility(plan):
    return 1 / len(plan)


def find_config_written(plan, current_configuration,
                        resource_string, cost_of_columns):
    overall_config_written, new_config = find_config_written_for_configuration(
        plan[0], current_configuration, resource_string, cost_of_columns)
    for run_i in range(len(plan) - 1):
        config_written, new_config = find_config_written_for_configuration(
            plan[run_i + 1], new_config, resource_string, cost_of_columns)
        overall_config_written += config_written
    return overall_config_written, new_config


def find_config_written_for_configuration(
        next_configuration, current_configuration, resource_string, cost_of_columns):
    written_frames = [0] * len(resource_string)
    fully_written_frames = []
    for column in resource_string:
        fully_written_frames.append(cost_of_columns[column])

    # Get complements of configs.
    reduced_next_config = list(next_configuration)
    reduced_current_config = list(current_configuration)
    for next_module in next_configuration:
        for cur_module in current_configuration:
            if cur_module.operation == next_module.operation and cur_module.bitstream == next_module.bitstream and cur_module.position == next_module.position:
                reduced_next_config.remove(next_module)
                reduced_current_config.remove(cur_module)

    # Get left over
    left_over_config = [
        module for module in current_configuration if module not in reduced_current_config]
    left_over_config.extend(reduced_next_config)

    find_new_written_frames(fully_written_frames,
                            written_frames, reduced_next_config)
    find_new_written_frames(fully_written_frames,
                            written_frames, reduced_current_config)

    return (sum(written_frames), tuple(left_over_config))


def find_new_written_frames(
        fully_written_frames, written_frames, configuration):
    for module in configuration:
        for column_i in range(module.position[0], module.position[1] + 1):
            written_frames[column_i] = fully_written_frames[column_i]


def max_utility_per_frames(all_unique_plans, last_configuration, resource_string,
                           utilites_scaler, config_written_scaler, utility_per_frame_scaler, cost_of_columns):
    utilites = []
    config_written = []
    for plan_i in range(len(all_unique_plans)):
        utilites.append(find_utility(all_unique_plans[plan_i]))
        config_written.append(
            find_config_written(all_unique_plans[plan_i], last_configuration, resource_string, cost_of_columns))
        # Plans with no frames written will be heavily preferred. Perhaps too
        # heavily?
        if (config_written[plan_i] == 0):
            config_written[plan_i] = 0.1
    utility_per_frame = []
    for plan_i in range(len(all_unique_plans)):
        utility_per_frame.append(utilites[plan_i] / config_written[plan_i])
    max_plan_i, score = find_best_scoring_plan(
        utilites, utilites_scaler, config_written, config_written_scaler, utility_per_frame, utility_per_frame_scaler)
    return (all_unique_plans[max_plan_i],
            utilites[max_plan_i], config_written[max_plan_i], score)


def find_best_scoring_plan(utilites, utilites_scaler, config_written,
                           config_written_scaler, utility_per_frame, utility_per_frame_scaler):
    minimised_config_written = [1 / i for i in config_written]
    max_util = max(utilites)
    max_frames = max(minimised_config_written)
    max_ratio = max(utility_per_frame)
    utilites_norm = [float(i) / max_util for i in utilites]
    frames_norm = [float(i) / max_frames
                   for i in minimised_config_written]
    utilites_per_frames_norm = [
        float(i) / max_ratio for i in utility_per_frame]
    scores = []
    for plan_i in range(len(utilites_norm)):
        scores.append(utilites_norm[plan_i] * utilites_scaler + frames_norm[plan_i] *
                      config_written_scaler + utilites_per_frames_norm[plan_i] * utility_per_frame_scaler)
    return scores.index(max(scores)), max(scores)


def find_fastest_plan(data_streamed, configuration_data_written,
                      streaming_speed, configuration_speed):
    runtimes = []
    exec_times = []
    config_times = []
    for plan_i in range(len(data_streamed)):
        runtimes.append(data_streamed[plan_i] / streaming_speed +
                        configuration_data_written[plan_i] / configuration_speed)
        exec_times.append(data_streamed[plan_i] / streaming_speed)
        config_times.append(
            configuration_data_written[plan_i] / configuration_speed)
    fastest_plan_i = runtimes.index(min(runtimes))
    return fastest_plan_i, exec_times[fastest_plan_i], config_times[fastest_plan_i]


def lowest_runtime(all_unique_plans, last_configuration, resource_string,
                   streaming_speed, configuration_speed, plan_metadata, cost_of_columns):
    data_streamed = []
    configuration_data_written = []
    for plan_i in range(len(all_unique_plans)):
        data_streamed.append(
            plan_metadata[all_unique_plans[plan_i]]["streamed_data_size"])
        config_written, new_config = find_config_written(
            all_unique_plans[plan_i], last_configuration, resource_string, cost_of_columns)
        configuration_data_written.append(
            config_written)
        plan_metadata[all_unique_plans[plan_i]
                      ]["last_configuration"] = new_config
    max_plan_i, exec_time, config_time = find_fastest_plan(
        data_streamed, configuration_data_written, streaming_speed, configuration_speed)
    # print("Configuration data:")
    # print(configuration_data_written[max_plan_i])
    return (all_unique_plans[max_plan_i], exec_time, config_time)


def choose_best_plan(all_unique_plans, smallest_run_count, last_configuration, resource_string, utilites_scaler, config_written_scaler,
                     utility_per_frame_scaler, plan_metadata, cost_of_columns, streaming_speed, configuration_speed, calculate_exact):
    # best_plan = min_runs_max_nodes_min_columns(all_unique_plans,smallest_run_count)

    cost_evaluation_start = perf_counter()

    if calculate_exact:
        best_plan = lowest_runtime(
            all_unique_plans, last_configuration, resource_string, streaming_speed, configuration_speed, plan_metadata, cost_of_columns)
    else:
        best_plan = max_utility_per_frames(
            all_unique_plans, last_configuration, resource_string, utilites_scaler, config_written_scaler, utility_per_frame_scaler, cost_of_columns)
    if not best_plan:
        raise ValueError("No best plan chosen!")

    stop_time = perf_counter()

    # print(
    # f"Time spent on cost evaluation : {stop_time -
    # cost_evaluation_start:.3f}s")

    return best_plan, stop_time - cost_evaluation_start


# For later comparison - to check if the plans differ? and how big is the difference of utility_per_frames
# This algo doesn't really make much sense. Why max nodes? Just
# performance wise fastest I guess?
def min_runs_max_nodes_min_columns(all_plans, smallest_run_count):
    best_plan = None
    max_nodes_in_min_plan = 0
    min_configured_rows_in_min_plan = 0
    for plan_i in range(len(all_plans)):
        if len(all_plans[plan_i]) == smallest_run_count:
            configured_rows = 0
            unique_node_names = set()
            for run_i in range(len(all_plans[plan_i])):
                for module in all_plans[plan_i][run_i]:
                    unique_node_names.add(module.node_name)
                    configured_rows += module.position[1] - \
                        module.position[0] + 1
            if len(unique_node_names) > max_nodes_in_min_plan:
                max_nodes_in_min_plan = len(unique_node_names)
                min_configured_rows_in_min_plan = configured_rows
                best_plan = all_plans[plan_i]
            elif len(unique_node_names) == max_nodes_in_min_plan and configured_rows < min_configured_rows_in_min_plan:
                min_configured_rows_in_min_plan = configured_rows
                best_plan = all_plans[plan_i]
    return best_plan


def max_utility():
    pass


def min_config_written():
    pass

# ----------- MAIN -----------


def main(argv):
    resource_string = "MMDMDBMMDBMMDMDBMMDBMMDMDBMMDBM"
    # == HARDWARE ==
    hw_library = {
        "Filter": {
            "bitstreams": {
                "filter81.bit": {"locations": (0, 10, 20), "length": 4, "capacity": (8, 1), "string": "MMDM",
                                 "is_backwards": False},
                "filter84.bit": {"locations": (2, 12, 22), "length": 7, "capacity": (8, 4), "string": "DMDBMMD",
                                 "is_backwards": False},
                "filter162.bit": {"locations": (0, 10, 20), "length": 5, "capacity": (16, 2), "string": "MMDMD",
                                  "is_backwards": False},
                "filter164.bit": {"locations": (0, 10, 20), "length": 7, "capacity": (16, 4), "string": "MMDMDBM",
                                  "is_backwards": False},
                "filter321.bit": {"locations": (0, 10, 20), "length": 5, "capacity": (32, 1), "string": "MMDMD",
                                  "is_backwards": False},
                "filter322.bit": {"locations": (2, 12, 22), "length": 7, "capacity": (32, 2), "string": "DMDBMMD",
                                  "is_backwards": False},
                "filter324.bit": {"locations": (0, 10, 20), "length": 10, "capacity": (32, 4), "string": "MMDMDBMMDB",
                                  "is_backwards": False}},
            "start_locations": [['filter81.bit', 'filter162.bit', 'filter164.bit', 'filter321.bit', 'filter324.bit'],
                                [], ['filter84.bit', 'filter322.bit'], [], [], [], [], [], [], [],
                                ['filter81.bit', 'filter162.bit', 'filter164.bit',
                                    'filter321.bit', 'filter324.bit'],
                                [], ['filter84.bit', 'filter322.bit'], [], [], [], [], [], [], [],
                                ['filter81.bit', 'filter162.bit', 'filter164.bit',
                                    'filter321.bit', 'filter324.bit'],
                                [], ['filter84.bit', 'filter322.bit'], [], [], [], [], [], [], [], []],
            "decorators": ["composable", "reducing"]
        }, "Linear Sort": {
            "bitstreams": {
                "linear512.bit": {"locations": (0, 10, 20), "length": 10, "capacity": (512,), "string": "MMDMDBMMDB",
                                  "is_backwards": False},
                "linear1024.bit": {"locations": (6, 16), "length": 14, "capacity": (1024,), "string": "MMDBMMDMDBMMDB",
                                   "is_backwards": False}},
            "start_locations": [['linear512.bit'], [], [], [], [], [], ['linear1024.bit'], [], [], [],
                                ['linear512.bit'], [],
                                [], [], [], [], ['linear1024.bit'], [], [], [], [
                                    'linear512.bit'], [], [], [], [], [],
                                [],
                                [], [], [], []],
            "decorators": ["sorting", "partial_sort"]
        }, "Merge Sort": {
            "bitstreams": {
                "mergesort32.bit": {"locations": (0, 10, 20), "length": 7, "capacity": (32,), "string": "MMDMDBM",
                                    "is_backwards": False},
                "mergesort64.bit": {"locations": (0, 10, 20), "length": 10, "capacity": (64,), "string": "MMDMDBMMDB",
                                    "is_backwards": False},
                "mergesort128.bit": {"locations": (8, 18), "length": 12, "capacity": (128,), "string": "DBMMDMDBMMDB",
                                     "is_backwards": False}},
            "start_locations": [['mergesort32.bit', 'mergesort64.bit'], [], [], [], [], [], [], [],
                                ['mergesort128.bit'], [],
                                ['mergesort32.bit', 'mergesort64.bit'], [
            ], [], [], [], [], [], [],
                ['mergesort128.bit'], [],
                ['mergesort32.bit', 'mergesort64.bit'], [], [], [], [], [], [], [], [], [], []],
            "decorators": ["composable", "blocking_sort", "sorting", "first_module"]
        }, "Merge Join": {
            "bitstreams": {
                "join.bit": {"locations": (0, 10, 20), "length": 6, "capacity": (), "string": "MMDMDB",
                             "is_backwards": False}},
            "start_locations": [['join.bit'], [], [], [], [], [], [], [], [], [], ['join.bit'], [], [], [], [], [], [],
                                [],
                                [], [], ['join.bit'], [], [], [], [], [], [], [], [], [], []],
            "decorators": ["sorted_input", "largest_input_is_output", "reducing"]
        }, "Addition": {
            "bitstreams": {
                "addition.bit": {"locations": (0, 10, 20), "length": 4, "capacity": (), "string": "MMDM",
                                 "is_backwards": False}},
            "start_locations": [['addition.bit'], [], [], [], [], [], [], [], [], [], ['addition.bit'], [], [], [], [],
                                [], [], [], [],
                                [], ['addition.bit'], [], [], [], [], [], [], [], [], [], []],
            "decorators": []
        }, "Multiplier": {
            "bitstreams": {
                "multiplier.bit": {"locations": (4, 14, 24), "length": 7, "capacity": (), "string": "DBMMDBM",
                                   "is_backwards": False}},
            "start_locations": [[], [], [], [], ['multiplier.bit'], [], [], [], [], [], [], [], [], [],
                                ['multiplier.bit'], [], [],
                                [], [], [], [], [], [], [], ['multiplier.bit'], [], [], [], [], [], []],
            "decorators": []
        }, "Global Sum": {
            "bitstreams": {
                "globalsum.bit": {"locations": (2, 12, 22), "length": 3, "capacity": (), "string": "DMD",
                                  "is_backwards": False}},
            "start_locations": [[], [], ['globalsum.bit'], [], [], [], [], [], [], [], [], [], ['globalsum.bit'], [],
                                [], [], [], [], [],
                                [], [], [], ['globalsum.bit'], [], [], [], [], [], [], [], []],
            "decorators": []
        }
    }
    test_hw = {
        "thing": {
            "bitstreams": {
                "multiplier.bit": {"locations": (0, 10, 20), "length": 7, "capacity": (), "string": "MBDMMBD",
                                   "is_backwards": False}},
            "start_locations": [['multiplier.bit'], [], [], [], [], [], [], [], [], [], ['multiplier.bit'], [], [], [],
                                [],
                                [], [], [], [], [], ['multiplier.bit'], [], [], [], [], [], [], [], [], [], []],
            "decorators": []
        }
    }

    # == Graphs ==
    graph = {
        "A": {"operation": "Addition", "capacity": (), "before": (("", -1),), "after": ("B",),
              "tables": ["test_table"], "satisfying_bitstreams": []},
        "B": {"operation": "Multiplier", "capacity": (), "before": (("A", 0),), "after": (""), "tables": [""],
              "satisfying_bitstreams": []},
        "C": {"operation": "Global Sum", "capacity": (), "before": (("", -1),), "after": (""), "tables": ["test_table2"],
              "satisfying_bitstreams": []}
    }
    # Double duplication for this actually. Duplicated once for 6 CMPs and
    # once double for 12 CMPs - 2nd Filter
    q19_graph = {
        "FirstFilter": {"operation": "Filter", "capacity": (4, 2), "before": (("", -1),), "after": ("LinSort",),
                        "tables": ["lineitem_1"], "satisfying_bitstreams": []},
        "LinSort": {"operation": "Linear Sort", "capacity": (), "before": (("FirstFilter", 0),),
                    "after": ("MergeSort",), "tables": [""], "satisfying_bitstreams": []},
        "MergeSort": {"operation": "Merge Sort", "capacity": (), "before": (("LinSort", 0),), "after": ("Join",),
                      "tables": [""], "satisfying_bitstreams": []},
        "Join": {"operation": "Merge Join", "capacity": (), "before": (("MergeSort", 0), ("", -1)), "after": ("SecondFilter",),
                 "tables": ["", "part_1"], "satisfying_bitstreams": []},
        "SecondFilter": {"operation": "Filter", "capacity": (12, 4), "before": (("Join", 0),), "after": ("Add",),
                         "tables": [""], "satisfying_bitstreams": []},
        "Add": {"operation": "Addition", "capacity": (), "before": (("SecondFilter", 0),), "after": ("Mul",),
                "tables": [""], "satisfying_bitstreams": []},
        "Mul": {"operation": "Multiplier", "capacity": (), "before": (("Add", 0),), "after": ("Sum",), "tables": [""],
                "satisfying_bitstreams": []},
        "Sum": {"operation": "Global Sum", "capacity": (), "before": (("Mul", 0),), "after": (""), "tables": [""],
                "satisfying_bitstreams": []}
    }
    capacity_test = {
        "FirstFilter": {"operation": "Filter", "capacity": (4, 2), "before": (("", -1),), "after": ("LinSort",),
                        "tables": ["test_table2"], "satisfying_bitstreams": []},
        "LinSort": {"operation": "Linear Sort", "capacity": (), "before": (("FirstFilter", 0),), "after": ("Sum",),
                    "tables": [""], "satisfying_bitstreams": []},
        "Sum": {"operation": "Global Sum", "capacity": (), "before": (("LinSort", 0),), "after": (""), "tables": [""],
                "satisfying_bitstreams": []}
    }
    filter_test = {
        "LinSort": {"operation": "Linear Sort", "capacity": (), "before": (("", -1),), "after": ("MergeSort",),
                    "tables": ["huge_table"], "satisfying_bitstreams": []},
        "MergeSort": {"operation": "Merge Sort", "capacity": (), "before": (("LinSort", 0),), "after": (""),
                      "tables": [""], "satisfying_bitstreams": []}}
    test_graph = {
        "first": {"operation": "thing", "capacity": (), "before": (("", -1),), "after": ("secondA", "secondB", "secondC"),
                  "tables": ["test_table", "test_table2"], "satisfying_bitstreams": []},
        "secondA": {"operation": "thing", "capacity": (), "before": (("first", 0),), "after": (""),
                    "tables": [""], "satisfying_bitstreams": []},
        "secondB": {"operation": "thing", "capacity": (), "before": (("first", 0),), "after": (""),
                    "tables": [""], "satisfying_bitstreams": []},
        "secondC": {"operation": "thing", "capacity": (), "before": (("first", 1),), "after": (""),
                    "tables": [""], "satisfying_bitstreams": []}}

    # == TABLES ==
    # Sorted sequence = ((begin, length, column_i),...); empty = unsorted

    my_sequence = []
    thing = 0
    while (thing < 128371):
        my_sequence.append(tuple([thing, 1024, 0]))
        # print(thing, thing+1024)
        thing += 1024

    my_sequence[-1] = tuple([my_sequence[-1][0],
                            128371 - my_sequence[-1][0], 0])

    tables = {
        "test_table": {"record_count": 10000, "sorted_sequences": ()},
        "test_table2": {"record_count": 10000, "sorted_sequences": ((0, 10000, 0),)},
        "huge_table": {"record_count": 210000, "sorted_sequences": ((0, 10, 0), (10, 20, 0),)},
        "lineitem_1": {"record_count": 6001215, "sorted_sequences": (), "record_size": 156},
        # Half-sorted
        "lineitem_2": {"record_count": 128371, "sorted_sequences": (tuple(my_sequence)), "record_size": 28},
        # Sorted
        "part_1": {"record_count": 200000, "sorted_sequences": ((0, 200000, 0),), "record_size": 176},
        "lineitem_part": {"record_count": 121, "sorted_sequences": ((0, 121, 0),), "record_size": 28}
    }

    q19_graph_1 = {
        "FirstFilter": {"operation": "Filter", "capacity": (4, 2), "before": (("", -1),), "after": ("LinSort",),
                        "tables": ["lineitem_1"], "satisfying_bitstreams": []},
        "LinSort": {"operation": "Linear Sort", "capacity": (), "before": (("FirstFilter", 0),),
                    "after": ("",), "tables": [""], "satisfying_bitstreams": []},
    }

    q19_graph_2 = {
        "MergeSort": {"operation": "Merge Sort", "capacity": (), "before": (("", -1),), "after": ("Join",),
                      "tables": ["lineitem_2"], "satisfying_bitstreams": []},
        "Join": {"operation": "Merge Join", "capacity": (), "before": (("MergeSort", 0), ("", -1)), "after": ("",),
                 "tables": ["", "part_1"], "satisfying_bitstreams": []},
    }

    q19_graph_3 = {
        "SecondFilter": {"operation": "Filter", "capacity": (12, 4), "before": (("", -1),), "after": ("Add",),
                         "tables": ["lineitem_part"], "satisfying_bitstreams": []},
        "Add": {"operation": "Addition", "capacity": (), "before": (("SecondFilter", 0),), "after": ("Mul",),
                "tables": [""], "satisfying_bitstreams": []},
        "Mul": {"operation": "Multiplier", "capacity": (), "before": (("Add", 0),), "after": ("Sum",), "tables": [""],
                "satisfying_bitstreams": []},
        "Sum": {"operation": "Global Sum", "capacity": (), "before": (("Mul", 0),), "after": (""), "tables": [""],
                "satisfying_bitstreams": []}
    }

    # First selection is for fitting, the second selection for the rest
    heuristics = [
        ([[ModuleSelection.LONGEST, ModuleSelection.FIRST_AVAILABLE]], [
         [ModuleSelection.SHORTEST, ModuleSelection.FIRST_AVAILABLE]]),
        ([[ModuleSelection.LONGEST]], [[ModuleSelection.SHORTEST]]),
        ([[ModuleSelection.LONGEST]], [[ModuleSelection.SHORTEST]]),
        ([[ModuleSelection.ALL_AVAILABLE]], [[ModuleSelection.ALL_AVAILABLE]]),
        ([[ModuleSelection.ALL_AVAILABLE]],)]
    default_selection = ([[ModuleSelection.SHORTEST]], [
        [ModuleSelection.LONGEST]])
    # starting_nodes = ["A", "C"]
    # find_plans_and_print(starting_nodes, graph, resource_string, hw_library, tables, [], default_selection)
    # capacity_test_nodes = ["FirstFilter"]
    # find_plans_and_print(capacity_test_nodes, capacity_test,
    #                      resource_string, hw_library, tables, [], default_selection)
    # filter_test_nodes = ["LinSort"]
    # find_plans_and_print(filter_test_nodes, filter_test,
    #                      resource_string, hw_library, tables, [], default_selection)
    #
    # q19_starting_nodes = ["FirstFilter"]
    # find_plans_and_print(q19_starting_nodes, q19_graph,
    #                      resource_string, hw_library, tables, [], default_selection, selectivity)
    # find_plans_and_print(["first"], test_graph,
    # resource_string, test_hw, tables, [], default_selection)

    with open(argv[1]) as graph_json:
        input_graph = json.load(graph_json)

    starting_nodes = set()
    processed_input_graph = {}
    for node_name, node_parameters in input_graph.items():
        before_list = [
            tuple(value) for value in node_parameters["before"]]
        if any(before_node[1] == -
               1 for before_node in node_parameters["before"]):
            starting_nodes.add(node_name)
        processed_input_graph[node_name] = {"operation": node_parameters["operation"], "capacity": tuple(node_parameters["capacity"]), "before": tuple(before_list), "after": tuple(node_parameters["after"]),
                                            "tables": node_parameters["tables"], "satisfying_bitstreams": node_parameters["satisfying_bitstreams"]}
    # print(node_name)
    # print(node_parameters)

    with open(argv[2]) as table_json:
        input_tables = json.load(table_json)

    processed_input_tables = {}
    for table_name, table_parameters in input_tables.items():
        processed_input_tables[table_name] = {
            "record_count": table_parameters["record_count"], "sorted_sequences": tuple(table_parameters["sorted_sequences"]), "record_size": table_parameters["record_size"]}
    # print(table_name)
    # print(table_parameters)

    with open(argv[0], "r") as file:
        for last_line in file:
            pass
    values = last_line.split(',')

    selectivity = float(values[-6])
    time_limit = float(values[-5])
    heuristic = int(values[-4])
    utility_scaler = float(values[-3])
    frame_scaler = float(values[-2])
    utility_per_frame_scaler = float(values[-1])

    # selectivity = 0.021

    # Smallest possible configuration:
    # filter + linear + mergesort+ join + filter + addition + multiplier +
    # globalsum = 315456 + 770784 + 770784 + 462768 + 315456 + 315456 + 916608
    # + 229152 = 4096464/66000000 = 0.06206763636

    # heuristic = 0
    # Don't care
    # utility_scaler = 1
    # frame_scaler = 1
    # utility_per_frame_scaler = 1

    disallow_empty_runs = True
    use_max_runs_cap = True
    if heuristic == 2 or heuristic == 3 or heuristic == 4:
        disallow_empty_runs = False
        use_max_runs_cap = False

    # q19_starting_nodes = set(["FirstFilter"])

    # starting_nodes = q19_starting_nodes
    # processed_input_graph = q19_graph
    # processed_input_tables = tables

    # Hardcoded for now.
    config_speed = 66000000
    streaming_speed = 4800000000
    frame_size = 372
    resource_column_costs = {'M': 216 * frame_size,
                             'D': 200 * frame_size, 'B': 196 * frame_size}
    operation_costs = {"Filter": 315456, "Linear Sort": 770784, "Merge Sort": 770784,
                       "Merge Join": 462768, "Addition": 315456, "Multiplier": 916608, "Global Sum": 229152}

    if time_limit == -1:
        time_limit = calculate_time_limit(
            processed_input_graph, processed_input_tables, config_speed, streaming_speed, operation_costs)

    calculate_exact = True
    fail_on_timeout = False

    converted_list = find_plans_and_print(starting_nodes, processed_input_graph,
                                          resource_string, hw_library, processed_input_tables, [
                                          ], heuristics[heuristic], selectivity,
                                          time_limit, utility_scaler, frame_scaler, utility_per_frame_scaler, disallow_empty_runs,
                                          use_max_runs_cap, resource_column_costs, config_speed, streaming_speed, calculate_exact, fail_on_timeout)

    # print()
    # print("Separate:")

    # find_plans_and_print(set(["FirstFilter"]), q19_graph_1,
    #                      resource_string, hw_library, processed_input_tables, [], heuristics[heuristic], selectivity, time_limit, utility_scaler, frame_scaler, utility_per_frame_scaler, disallow_empty_runs, use_max_runs_cap)
    # find_plans_and_print(set(["MergeSort"]), q19_graph_2,
    #                      resource_string, hw_library, processed_input_tables, [], heuristics[heuristic], selectivity, time_limit, utility_scaler, frame_scaler, utility_per_frame_scaler, disallow_empty_runs, use_max_runs_cap)
    # find_plans_and_print(set(["SecondFilter"]), q19_graph_3,
    # resource_string, hw_library, processed_input_tables, [],
    # heuristics[heuristic], selectivity, time_limit, utility_scaler,
    # frame_scaler, utility_per_frame_scaler, disallow_empty_runs,
    # use_max_runs_cap)

    with open(argv[0], "a") as stats_file:
        stats_file.write(",")
        stats_file.write(",".join(converted_list))


def calculate_time_limit(processed_input_graph, processed_input_tables,
                         config_speed, streaming_speed, operation_costs):
    smallest_config_size = 0
    for node_name in processed_input_graph.keys():
        smallest_config_size += operation_costs[processed_input_graph[node_name]["operation"]]
    config_time = smallest_config_size / config_speed
    table_sizes = 0
    for node_name in processed_input_graph.keys():
        for table in processed_input_graph[node_name]["tables"]:
            if table:
                table_sizes += processed_input_tables[table]["record_count"] * \
                    processed_input_tables[table]["record_size"]
    exec_time = table_sizes / streaming_speed
    return config_time + exec_time


if __name__ == '__main__':
    main(sys.argv[1:])
    # Missing parts:
    # 1. Backwards paths
    # Optimisation rules:
    #   Reordering
    #   Data duplication for filters
    #   Actually linear sort removal is also an option
    # Fixing filter and sorting parameters
