#include "dma_crossbar_setup.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include "dma_setup_data.hpp"
#include "query_acceleration_constants.hpp"
namespace {
const int kTestAnyChunk = 0;
const int kTestAnyPosition = 1;
const int kDatapathLength = query_acceleration_constants::kDatapathLength;
const int kDatapathWidth = query_acceleration_constants::kDatapathWidth;

void GetGoldenConfigFromFile(std::vector<std::vector<int>>& golden_config,
                             const std::string& file_name) {
  std::ifstream input_file(file_name);
  ASSERT_TRUE(input_file);

  std::string line;
  while (std::getline(input_file, line)) {
    std::istringstream string_stream(line);
    int config_value = 0;
    std::vector<int> current_cycle_golden_config;
    while (string_stream >> config_value) {
      current_cycle_golden_config.push_back(config_value);
    }
    golden_config.push_back(current_cycle_golden_config);
  }
}

auto CreateLinearSelectedColumnsVector(const int vector_size)
    -> std::vector<int> {
  std::vector<int> selected_column(vector_size);
  std::generate(selected_column.begin(), selected_column.end(),
                [n = 0]() mutable { return n++; });
  return selected_column;
}

void ExpectConfigurationDataIsUnconfigured(DMASetupData configuration_data) {
  for (int clock_cycle_index = 0; clock_cycle_index < kDatapathLength;
       clock_cycle_index++) {
    EXPECT_THAT(configuration_data.crossbar_setup_data.size(), testing::Eq(0));
  }
}

void ExpectConfigurationDataIsConfigured(
    DMASetupData configuration_data, std::string golden_chunk_data_file,
    std::string golden_position_data_file) {
  std::vector<std::vector<int>> golden_chunk_config;
  GetGoldenConfigFromFile(golden_chunk_config, golden_chunk_data_file);
  std::vector<std::vector<int>> golden_position_config;
  GetGoldenConfigFromFile(golden_position_config, golden_position_data_file);
  for (int clock_cycle_index = 0; clock_cycle_index < kDatapathLength;
       clock_cycle_index++) {
    EXPECT_THAT(
        configuration_data.crossbar_setup_data[clock_cycle_index].chunk_data,
        testing::ElementsAreArray(golden_chunk_config[clock_cycle_index]))
        << "Vectors differ at clock cycle: " << clock_cycle_index;
    EXPECT_THAT(
        configuration_data.crossbar_setup_data[clock_cycle_index].position_data,
        testing::ElementsAreArray(golden_position_config[clock_cycle_index]))
        << "Vectors differ at clock cycle: " << clock_cycle_index;
  }
}

TEST(DMACrossbarSetupTest, RecordSize18BufferToInterfaceSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = true;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data,
      18, 
      CreateLinearSelectedColumnsVector(18));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize18BufferToInterfaceChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize18BufferToInterfacePositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize18InterfaceToBufferSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 18,
      CreateLinearSelectedColumnsVector(18));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize18InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize18InterfaceToBufferPositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize4BufferToInterfaceSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = true;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 4,
      CreateLinearSelectedColumnsVector(4));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize4BufferToInterfaceChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize4BufferToInterfacePositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize4InterfaceToBufferSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 4,
      CreateLinearSelectedColumnsVector(4));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize4InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize4InterfaceToBufferPositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize46BufferToInterfaceSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = true;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 46,
      CreateLinearSelectedColumnsVector(46));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize46BufferToInterfaceChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize46BufferToInterfacePositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize46InterfaceToBufferSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 46,
      CreateLinearSelectedColumnsVector(46));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize46InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize46InterfaceToBufferPositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize57BufferToInterfaceSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = true;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 57,
      CreateLinearSelectedColumnsVector(57));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize57BufferToInterfaceChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize57BufferToInterfacePositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize57InterfaceToBufferSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 57,
      CreateLinearSelectedColumnsVector(57));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize57InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize57InterfaceToBufferPositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize478BufferToInterfaceSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = true;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 478,
      CreateLinearSelectedColumnsVector(478));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize478BufferToInterfaceChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize478BufferToInterfacePositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize478InterfaceToBufferSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 478,
      CreateLinearSelectedColumnsVector(478));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize478InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize478InterfaceToBufferPositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize80BufferToInterfaceSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = true;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 80,
      CreateLinearSelectedColumnsVector(80));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize80BufferToInterfaceChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize80BufferToInterfacePositionSetup.txt");
}

TEST(DMACrossbarSetupTest, RecordSize80InterfaceToBufferSetupCheck) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 80,
      CreateLinearSelectedColumnsVector(80));

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize80InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize80InterfaceToBufferPositionSetup.txt");
}

TEST(DMACrossbarSetupTest, thing) {
  DMASetupData test_stream_setup_data;
  test_stream_setup_data.is_input_stream = false;
  ExpectConfigurationDataIsUnconfigured(test_stream_setup_data);

  //DMACrossbarSetup::CalculateCrossbarSetupData(
  //    kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 16, {5,5,5,5,5,3,3,3,4,4,4,4});

  DMACrossbarSetup::CalculateCrossbarSetupData(
      kTestAnyChunk, kTestAnyPosition, test_stream_setup_data, 16, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15});

  ExpectConfigurationDataIsConfigured(
      test_stream_setup_data,
      "DMACrossbarSetupTest/RecordSize80InterfaceToBufferChunkSetup.txt",
      "DMACrossbarSetupTest/RecordSize80InterfaceToBufferPositionSetup.txt");
}

}  // namespace
