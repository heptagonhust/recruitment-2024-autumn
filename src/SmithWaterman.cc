#include "SmithWaterman.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

SmithWaterman::SmithWaterman(const std::string& query_seq_path,
                             const std::string& target_seq_path) {
  read_seq(query_seq_path, query_seqs);
  read_seq(target_seq_path, target_seqs);

  query_seqs_size = query_seqs.size();
  assert(query_seqs_size >= 1);
  target_seqs_size = target_seqs.size();
  assert(target_seqs_size >= 1);
}

std::vector<size_t> SmithWaterman::solve() {
  // Iterate through the query sequences
  for (auto& query_seq : query_seqs) {
    size_t query_seq_length = query_seq.sequence.size();

    // Iterate throuth the target sequences
    for (auto& target_seq : target_seqs) {
      size_t target_seq_length = target_seq.sequence.size();
      // Pairwise-Alignment
      pair_align(query_seq, target_seq, query_seq_length, target_seq_length);
    }
  }

  return max_scores;
}

void SmithWaterman::report() const {
  for (size_t i = 0; i < query_seqs_size; i++) {
    auto max_score_ptr =
        std::max_element(max_scores.cbegin() + i * target_seqs_size,
                         max_scores.cbegin() + (i + 1) * target_seqs_size);
    size_t max_score_idx = std::distance(max_scores.cbegin(), max_score_ptr);
    std::cout << "Current query sequence: " << query_seqs.at(i).description
              << std::endl;
    std::cout << "The most similar sequence: "
              << target_seqs.at(max_score_idx % target_seqs_size).description
              << std::endl;
    std::cout << "The Simiarity Score: " << *max_score_ptr << std::endl
              << std::endl;
  }
}

void SmithWaterman::pair_align(FastaSequence& query_seq,
                               FastaSequence& target_seq,
                               size_t query_seq_length,
                               size_t target_seq_length) {
  size_t pad = target_seq_length + 1;
  // Resize the similarity_matrix
  H.resize((query_seq_length + 1) * (target_seq_length + 1), 0);

  // Store the highest score in each pairwise-alignment process.
  // Default to 0.
  max_positions.push_back(0);

  // Pairwise-Alignment between the two sequences
  for (int64_t i = 1; i <= query_seq_length; i++) {
    for (int64_t j = 1; j <= target_seq_length; j++) {
      int64_t index = pad * i + j;

      // From the upper element
      int64_t up = H[index - pad] + gap_score;

      // From the left element
      int64_t left = H[index - 1] + gap_score;

      // From the upper-left element
      int64_t upleft =
          H[index - pad - 1] +
          (query_seq.sequence.at(i - 1) == target_seq.sequence.at(j - 1)
               ? match_score
               : mismatch_score);

      int64_t max = std::max({up, left, upleft, 0l});

      H[index] = max;

      if (max > H[max_positions.back()]) {
        max_positions.back() = index;
      }
    }
  }
  max_scores.push_back(H[max_positions.back()]);
}

int SmithWaterman::validate(const std::string& ref_path) {
  read_ref(ref_path, refs);
  if (refs == max_scores) {
    std::cout << "Result correct!!!" << std::endl;
    report();
    return 0;
  } else {
    std::cout << "Result not match!!!" << std::endl;
    std::cout << "Reference Scores: ";
    std::copy(refs.cbegin(), refs.cend(),
              std::ostream_iterator<size_t>(std::cout, " "));
    std::cout << std::endl;

    std::cout << "Calculated Scores: ";
    std::copy(max_scores.cbegin(), max_scores.cend(),
              std::ostream_iterator<size_t>(std::cout, " "));
    std::cout << std::endl;

    return -1;
  }
}

void SmithWaterman::read_seq(const std::string& seq_path,
                             std::vector<FastaSequence>& seqs) {
  // Open file
  std::ifstream seq_file{seq_path};
  if (!seq_file.is_open()) {
    std::cerr << "Error opening file: " << seq_path << std::endl;
    throw std::runtime_error("Failed to open sequence file");
  }

  std::string line;
  FastaSequence curr_seq;

  // Read seqs
  while (std::getline(seq_file, line)) {
    if (line[0] == '>') {
      if (!curr_seq.sequence.empty()) {
        seqs.push_back(curr_seq);
        curr_seq = FastaSequence();
      }
      curr_seq.description = line.substr(1);
    } else {
      curr_seq.sequence += line;
    }
  }
  if (!curr_seq.sequence.empty()) {
    seqs.push_back(curr_seq);
  }

  // Close file
  seq_file.close();
}

void SmithWaterman::read_ref(const std::string& ref_path,
                             std::vector<size_t>& refs) {
  std::ifstream ref_file{ref_path};
  if (!ref_file.is_open()) {
    std::cerr << "Error opening the reference file: " << ref_path << std::endl;
    throw std::runtime_error("Failed to open reference file");
  }

  std::string line;
  while (std::getline(ref_file, line)) {
    std::istringstream iss(line);
    size_t score;
    while (iss >> score) {
      refs.push_back(score);
    }
  }
  ref_file.close();
}
