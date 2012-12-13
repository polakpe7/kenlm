#include "lm/builder/pipeline.hh"

#include "lm/builder/adjust_counts.hh"
#include "lm/builder/corpus_count.hh"
#include "lm/builder/multi_sort.hh"
#include "lm/builder/print.hh"
#include "lm/builder/sort.hh"

#include <vector>

namespace lm { namespace builder {

void Pipeline(const PipelineConfig &config, util::FilePiece &text, std::ostream &out) {
  std::vector<util::stream::ChainConfig> chain_configs(config.order, config.chain);
  for (std::size_t i = 0; i < config.order; ++i) {
    chain_configs[i].entry_size = NGram::TotalSize(i + 1);
  }

  util::stream::Sort<SuffixOrder, AddCombiner> first_suffix(config.sort, SuffixOrder(config.order));
  util::stream::Chain(chain_configs.back()) >> CorpusCount(text, config.order, config.vocab_file) >> first_suffix.Unsorted();

  std::vector<uint64_t> counts;
  std::vector<Discount> discounts;
  Sorts<ContextOrder> second_context(config.sort);
  {
    Chains chains(chain_configs);
    chains[config.order - 1] >> first_suffix.Sorted();
    chains >> AdjustCounts(counts, discounts) >> second_context.Unsorted();
  }
  VocabReconstitute vocab(config.vocab_file.c_str());
  Chains chains(chain_configs);
  chains >> second_context.Sorted() >> Print<uint64_t>(vocab, std::cout) >> util::stream::kRecycle;
}

}} // namespaces
