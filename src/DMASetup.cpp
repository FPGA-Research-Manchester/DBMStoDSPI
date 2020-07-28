#include "DMASetup.hpp"

#include <cmath>
#include <cstdio>
#include <queue>
#include <tuple>

#include "DMACrossbarSetup.hpp"

void DMASetup::SetupDMAModule(DMAInterface& dma_engine,
                              std::vector<int>& db_data, int record_size,
                              int record_count, int input_stream_id,
                              int output_stream_id) {
  // Calculate the controller parameter values based on input data and datatypes
  // Every size metric is 1 integer = 4 bytes = 32 bits
  const int max_ddr_burst_size = 512;
  const int max_chunk_size = 16;
  const int max_ddr_size_per_cycle = 4;

  // Input
  DMASetupData input_stream_setup_data;
  input_stream_setup_data.streamID = input_stream_id;
  input_stream_setup_data.isInputStream = true;
  input_stream_setup_data.recordCount = record_count;
  // inputStreamSetupData.recordCount = doc.GetRowCount();
  CalculateDMAStreamSetupData(input_stream_setup_data, max_chunk_size,
                              max_ddr_burst_size, max_ddr_size_per_cycle,
                              db_data, record_size);

  // Output
  DMASetupData output_stream_setup_data;
  output_stream_setup_data.streamID = output_stream_id;
  output_stream_setup_data.isInputStream = false;
  output_stream_setup_data.recordCount = 0;
  CalculateDMAStreamSetupData(output_stream_setup_data, max_chunk_size,
                              max_ddr_burst_size, max_ddr_size_per_cycle,
                              db_data, record_size);

  const int any_chunk = 31;
  const int any_position = 3;
  DMACrossbarSetup crossbar_configuration_finder;
  DMACrossbarSetup::FindInputCrossbarSetupData(any_chunk, any_position,
                                               input_stream_setup_data);
  DMACrossbarSetup::FindOutputCrossbarSetupData(any_chunk, any_position,
                                                output_stream_setup_data);

  std::vector<DMASetupData> setup_data_for_dma;
  setup_data_for_dma.push_back(input_stream_setup_data);
  setup_data_for_dma.push_back(output_stream_setup_data);
  WriteSetupDataToDMAModule(setup_data_for_dma, dma_engine);
}

void DMASetup::WriteSetupDataToDMAModule(
    std::vector<DMASetupData>& setup_data_for_dma, DMAInterface& dma_engine) {
  for (auto& stream_setup_data : setup_data_for_dma) {
    SetUpDMAIOStreams(stream_setup_data, dma_engine);
    SetUpDMACrossbars(stream_setup_data, dma_engine);
  }
}

void DMASetup::SetUpDMACrossbars(DMASetupData& stream_setup_data,
                                 DMAInterface& dma_engine) {
  for (size_t current_chunk_index = 0;
       current_chunk_index < stream_setup_data.crossbarSetupData.size();
       ++current_chunk_index) {
    if (stream_setup_data.isInputStream) {
      for (int current_offset = 0; current_offset < 4; current_offset++) {
        dma_engine.setBufferToInterfaceChunk(
            stream_setup_data.streamID, current_chunk_index, current_offset,
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[3 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[2 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[1 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[0 + current_offset * 4]);
        dma_engine.setBufferToInterfaceSourcePosition(
            stream_setup_data.streamID, current_chunk_index, current_offset,
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[3 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[2 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[1 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[0 + current_offset * 4]);
      }
    } else {
      for (int current_offset = 0; current_offset < 4; current_offset++) {
        dma_engine.setInterfaceToBufferChunk(
            stream_setup_data.streamID, current_chunk_index, current_offset,
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[3 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[2 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[1 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .chunkData[0 + current_offset * 4]);
        dma_engine.setInterfaceToBufferSourcePosition(
            stream_setup_data.streamID, current_chunk_index, current_offset,
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[3 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[2 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[1 + current_offset * 4],
            stream_setup_data.crossbarSetupData[current_chunk_index]
                .positionData[0 + current_offset * 4]);
      }
    }
  }
}

void DMASetup::SetUpDMAIOStreams(DMASetupData& stream_setup_data,
                                 DMAInterface& dma_engine) {
  if (stream_setup_data.isInputStream) {
    dma_engine.setInputControllerParams(
        stream_setup_data.streamID, stream_setup_data.DDRBurstLength,
        stream_setup_data.recordsPerDDRBurst, stream_setup_data.bufferStart,
        stream_setup_data.bufferEnd);
    dma_engine.setInputControllerStreamAddress(stream_setup_data.streamID,
                                               stream_setup_data.streamAddress);
    dma_engine.setInputControllerStreamSize(stream_setup_data.streamID,
                                            stream_setup_data.recordCount);
  } else {
    dma_engine.setOutputControllerParams(
        stream_setup_data.streamID, stream_setup_data.DDRBurstLength,
        stream_setup_data.recordsPerDDRBurst, stream_setup_data.bufferStart,
        stream_setup_data.bufferEnd);
    dma_engine.setOutputControllerStreamAddress(
        stream_setup_data.streamID, stream_setup_data.streamAddress);
    dma_engine.setOutputControllerStreamSize(stream_setup_data.streamID,
                                             stream_setup_data.recordCount);
  }
  dma_engine.setRecordSize(stream_setup_data.streamID,
                           stream_setup_data.chunksPerRecord);
  for (auto& chunk_id_pair : stream_setup_data.recordChunkIDs) {
    dma_engine.setRecordChunkIDs(stream_setup_data.streamID,
                                 std::get<0>(chunk_id_pair),
                                 std::get<1>(chunk_id_pair));
  }
}

void DMASetup::CalculateDMAStreamSetupData(DMASetupData& stream_setup_data,
                                           const int& max_chunk_size,
                                           const int& max_ddr_burst_size,
                                           const int& max_ddr_size_per_cycle,
                                           std::vector<int>& db_data,
                                           int record_size) {
  stream_setup_data.chunksPerRecord =
      (record_size + max_chunk_size - 1) / max_chunk_size;  // ceil

  // Temporarily for now.
  for (int i = 0; i < stream_setup_data.chunksPerRecord; i++) {
    stream_setup_data.recordChunkIDs.emplace_back(i, i);
  }

  int records_per_max_burst_size = max_ddr_burst_size / record_size;
  stream_setup_data.recordsPerDDRBurst =
      pow(2, static_cast<int>(log2(records_per_max_burst_size)));

  stream_setup_data.DDRBurstLength =
      ((record_size * stream_setup_data.recordsPerDDRBurst) +
       max_ddr_size_per_cycle - 1) /
      max_ddr_size_per_cycle;  // ceil (recordSize * recordsPerDDRBurst) /
                               // maxDDRSizePerCycle

  // Temporarily for now
  stream_setup_data.bufferStart = 0;
  stream_setup_data.bufferEnd = 15;

  stream_setup_data.streamAddress = reinterpret_cast<uintptr_t>(&db_data[0]);
}