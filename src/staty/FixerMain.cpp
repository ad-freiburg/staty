// Copyright 2019, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include <iostream>
#include "staty/index/StatIdx.h"
#include "staty/server/StatServer.h"
#include "util/http/Server.h"
#include "util/log/Log.h"

using staty::IdRange;
using staty::StatIdx;
using staty::StatServer;

// _____________________________________________________________________________
void printHelp(int argc, char** argv) {
  UNUSED(argc);
  std::cout << "Usage: " << argv[0] << " [-p <port>] [--help] [-h] <fix input files>" << "\n";
  std::cout << "\nAllowed arguments:\n    -p <port>  Port for server to listen to\n";
}

// _____________________________________________________________________________
int main(int argc, char** argv) {
  // disable output buffering for standard output
  setbuf(stdout, NULL);

  // initialize randomness
  srand(time(NULL) + rand());  // NOLINT

  int port = 9090;

  std::vector<std::string> inputPaths;

  for (int i = 1; i < argc; i++) {
    std::string cur = argv[i];
    if (cur == "-h" || cur == "--help") {
      printHelp(argc, argv);
      exit(0);
    } else if (cur == "-p") {
      if (++i >= argc) {
        LOG(ERROR) << "Missing argument for port (-p).";
        exit(1);
      }
      port = atoi(argv[i]);
    } else {
      inputPaths.push_back(cur);
    }
  }

  if (inputPaths.size() == 0) {
    LOG(ERROR) << "Missing path(s) to stations file(s).";
    printHelp(argc, argv);
    exit(1);
  }

  std::vector<std::pair<IdRange, StatIdx>> idx(inputPaths.size());
  IdRange idRange{0, 0, 0, 0, 0, 0};
  for (size_t i = 0; i < inputPaths.size(); i++) {
    LOG(INFO) << "Processing " << inputPaths[i];
    idx[i].first = idRange;
    if (i > 0) {
      idx[i].first.sidStart = idx[i - 1].first.sidEnd + 1;
      idx[i].first.gidStart = idx[i - 1].first.gidEnd + 1;
      idx[i].first.suggIdStart = idx[i - 1].first.suggIdStart + 1;
    }
    idx[i].second.readFromFile(inputPaths[i]);
    idx[i].first.sidEnd = idx[i].first.sidStart + idx[i].second.maxSId() + 1;
    idx[i].first.gidEnd = idx[i].first.gidStart + idx[i].second.maxGId() + 1;
    idx[i].first.suggIdEnd =
        idx[i].first.suggIdStart + idx[i].second.maxSuggId() + 1;
  }

  LOG(INFO) << "Building search index...";
  staty::SearchIdx searchIdx(idx);

  LOG(INFO) << "Starting server...";
  StatServer serv(idx, searchIdx);

  LOG(INFO) << "Listening on port " << port;
  util::http::HttpServer(port, &serv, std::thread::hardware_concurrency()).run();
}

