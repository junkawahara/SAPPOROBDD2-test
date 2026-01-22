/**
 * @file zdd_iterators.cpp
 * @brief SAPPOROBDD 2.0 - ZDDイテレータ実装
 * @author SAPPOROBDD Team
 * @copyright MIT License
 */

#include "sbdd2/zdd.hpp"
#include "sbdd2/zdd_iterators.hpp"

namespace sbdd2 {

// ============== DictIterator ==============

DictIterator::DictIterator(const ZDD* zdd, int64_t count, bool reverse)
    : zdd_(zdd), count_(count), reverse_(reverse), is_end_(false)
{
    if (count == 0) {
        is_end_ = true;
        current_ = 0;
    } else if (reverse) {
        current_ = count;  // Start at count, will access index count-1
    } else {
        current_ = 0;  // Start at 0
    }
}

bool DictIterator::at_end() const {
    if (is_end_) return true;
    if (reverse_) {
        return current_ == 0;
    } else {
        return current_ >= count_;
    }
}

std::set<bddvar> DictIterator::operator*() const {
    if (zdd_ == nullptr || at_end()) {
        return std::set<bddvar>();
    }
    if (reverse_) {
        // For reverse, current_ goes from count_ down to 1
        // We want to return sets from index count_-1 down to 0
        return zdd_->get_set(current_ - 1);
    } else {
        return zdd_->get_set(current_);
    }
}

DictIterator& DictIterator::operator++() {
    if (at_end()) {
        return *this;
    }
    if (reverse_) {
        --current_;
    } else {
        ++current_;
    }
    return *this;
}

// ============== WeightIterator ==============

WeightIterator::WeightIterator()
    : remaining_(nullptr), is_min_(false)
{
}

WeightIterator::WeightIterator(const ZDD& zdd, const std::vector<int64_t>& weights, bool is_min)
    : remaining_(new ZDD(zdd)), weights_(weights), is_min_(is_min)
{
    updateCurrent();
}

WeightIterator::WeightIterator(const WeightIterator& other)
    : remaining_(other.remaining_ ? new ZDD(*other.remaining_) : nullptr),
      weights_(other.weights_),
      is_min_(other.is_min_),
      current_(other.current_)
{
}

WeightIterator::WeightIterator(WeightIterator&& other) noexcept
    : remaining_(other.remaining_),
      weights_(std::move(other.weights_)),
      is_min_(other.is_min_),
      current_(std::move(other.current_))
{
    other.remaining_ = nullptr;
}

WeightIterator& WeightIterator::operator=(const WeightIterator& other) {
    if (this != &other) {
        delete remaining_;
        remaining_ = other.remaining_ ? new ZDD(*other.remaining_) : nullptr;
        weights_ = other.weights_;
        is_min_ = other.is_min_;
        current_ = other.current_;
    }
    return *this;
}

WeightIterator& WeightIterator::operator=(WeightIterator&& other) noexcept {
    if (this != &other) {
        delete remaining_;
        remaining_ = other.remaining_;
        weights_ = std::move(other.weights_);
        is_min_ = other.is_min_;
        current_ = std::move(other.current_);
        other.remaining_ = nullptr;
    }
    return *this;
}

WeightIterator::~WeightIterator() {
    delete remaining_;
}

void WeightIterator::updateCurrent() {
    current_.clear();
    if (remaining_ == nullptr || remaining_->is_zero()) {
        return;
    }

    if (is_min_) {
        remaining_->min_weight(weights_, current_);
    } else {
        remaining_->max_weight(weights_, current_);
    }
}

WeightIterator& WeightIterator::operator++() {
    if (remaining_ == nullptr || remaining_->is_zero()) {
        return *this;
    }

    // Remove current set from remaining
    // Create a ZDD representing the single set {current_}
    ZDD single_set = ZDD::base(*remaining_->manager());
    for (bddvar v : current_) {
        ZDD var_zdd = ZDD::single(*remaining_->manager(), v);
        single_set = single_set.product(var_zdd);
    }

    // Remove from remaining
    *remaining_ = *remaining_ - single_set;

    // Update current
    updateCurrent();

    return *this;
}

bool WeightIterator::operator==(const WeightIterator& other) const {
    // Both are end iterators
    if ((remaining_ == nullptr || remaining_->is_zero()) &&
        (other.remaining_ == nullptr || other.remaining_->is_zero())) {
        return true;
    }
    // One is end, other is not
    if ((remaining_ == nullptr || remaining_->is_zero()) !=
        (other.remaining_ == nullptr || other.remaining_->is_zero())) {
        return false;
    }
    // Both have remaining elements - compare by remaining ZDD
    return *remaining_ == *other.remaining_;
}

// ============== RandomIterator ==============

RandomIterator::RandomIterator()
    : remaining_(nullptr), rng_(nullptr)
{
}

RandomIterator::RandomIterator(const ZDD& zdd, std::mt19937& rng)
    : remaining_(new ZDD(zdd)), rng_(&rng)
{
    updateCurrent();
}

RandomIterator::RandomIterator(const RandomIterator& other)
    : remaining_(other.remaining_ ? new ZDD(*other.remaining_) : nullptr),
      rng_(other.rng_),
      current_(other.current_)
{
}

RandomIterator::RandomIterator(RandomIterator&& other) noexcept
    : remaining_(other.remaining_),
      rng_(other.rng_),
      current_(std::move(other.current_))
{
    other.remaining_ = nullptr;
}

RandomIterator& RandomIterator::operator=(const RandomIterator& other) {
    if (this != &other) {
        delete remaining_;
        remaining_ = other.remaining_ ? new ZDD(*other.remaining_) : nullptr;
        rng_ = other.rng_;
        current_ = other.current_;
    }
    return *this;
}

RandomIterator& RandomIterator::operator=(RandomIterator&& other) noexcept {
    if (this != &other) {
        delete remaining_;
        remaining_ = other.remaining_;
        rng_ = other.rng_;
        current_ = std::move(other.current_);
        other.remaining_ = nullptr;
    }
    return *this;
}

RandomIterator::~RandomIterator() {
    delete remaining_;
}

void RandomIterator::updateCurrent() {
    current_.clear();
    if (remaining_ == nullptr || remaining_->is_zero() || rng_ == nullptr) {
        return;
    }

    current_ = remaining_->sample_randomly(*rng_);
}

RandomIterator& RandomIterator::operator++() {
    if (remaining_ == nullptr || remaining_->is_zero()) {
        return *this;
    }

    // Remove current set from remaining
    ZDD single_set = ZDD::base(*remaining_->manager());
    for (bddvar v : current_) {
        ZDD var_zdd = ZDD::single(*remaining_->manager(), v);
        single_set = single_set.product(var_zdd);
    }

    *remaining_ = *remaining_ - single_set;

    // Update current
    updateCurrent();

    return *this;
}

bool RandomIterator::operator==(const RandomIterator& other) const {
    // Both are end iterators
    if ((remaining_ == nullptr || remaining_->is_zero()) &&
        (other.remaining_ == nullptr || other.remaining_->is_zero())) {
        return true;
    }
    // One is end, other is not
    if ((remaining_ == nullptr || remaining_->is_zero()) !=
        (other.remaining_ == nullptr || other.remaining_->is_zero())) {
        return false;
    }
    // Both have remaining elements
    return *remaining_ == *other.remaining_;
}

// ============== ZDD Iterator Methods ==============

DictIterator ZDD::dict_begin() const {
    int64_t count = static_cast<int64_t>(indexed_count());
    return DictIterator(this, count, false);
}

DictIterator ZDD::dict_end() const {
    // Use default constructor which creates an end iterator
    return DictIterator();
}

DictIterator ZDD::dict_rbegin() const {
    int64_t count = static_cast<int64_t>(indexed_count());
    return DictIterator(this, count, true);
}

DictIterator ZDD::dict_rend() const {
    // Use default constructor which creates an end iterator
    return DictIterator();
}

WeightIterator ZDD::weight_min_begin(const std::vector<int64_t>& weights) const {
    if (is_zero()) {
        return weight_min_end();
    }
    return WeightIterator(*this, weights, true);
}

WeightIterator ZDD::weight_min_end() const {
    return WeightIterator();
}

WeightIterator ZDD::weight_max_begin(const std::vector<int64_t>& weights) const {
    if (is_zero()) {
        return weight_max_end();
    }
    return WeightIterator(*this, weights, false);
}

WeightIterator ZDD::weight_max_end() const {
    return WeightIterator();
}

RandomIterator ZDD::random_begin(std::mt19937& rng) const {
    if (is_zero()) {
        return random_end();
    }
    return RandomIterator(*this, rng);
}

RandomIterator ZDD::random_end() const {
    return RandomIterator();
}

} // namespace sbdd2
