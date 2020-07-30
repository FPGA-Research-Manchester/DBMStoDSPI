#include "FilterSetup.hpp"

#include <cstdio>

void FilterSetup::SetupFilterModule(FilterInterface& filter_module,
                                    int input_stream_id, int output_stream_id) {
  filter_module.FilterSetStreamIDs(input_stream_id, output_stream_id, output_stream_id);

  bool request_on_invalid_if_last = true;
  bool forward_invalid_record_first_chunk = false;
  bool forward_full_invalid_records = false;

  bool first_module_in_resource_elastic_chain = true;
  bool last_module_in_resource_elastic_chain = true;

  filter_module.FilterSetMode(
      request_on_invalid_if_last, forward_invalid_record_first_chunk,
      forward_full_invalid_records, first_module_in_resource_elastic_chain,
      last_module_in_resource_elastic_chain);

  uint32_t chunk_id = 1;
  uint32_t data_position = 1;

  uint32_t const less_than_compare = 0;
  uint32_t const dont_care_compare = 0;

  filter_module.FilterSetCompareTypes(chunk_id, data_position,
                                      less_than_compare, dont_care_compare,
                                      dont_care_compare, dont_care_compare);

  uint32_t compare_lane_index = 0;
  uint32_t compare_reference_value = 12000;

  filter_module.FilterSetCompareReferenceValue(
      chunk_id, data_position, compare_lane_index, compare_reference_value);

  uint32_t dnf_clause_id = 0;
  uint8_t const positive_literal_type = 1;

  filter_module.FilterSetDNFClauseLiteral(dnf_clause_id, compare_lane_index,
                                          chunk_id, data_position,
                                          positive_literal_type);

  uint32_t datapath_width = 16;

  filter_module.WriteDNFClauseLiteralsToFilter_1CMP_8DNF(datapath_width);
}
