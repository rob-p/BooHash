#ifndef __BOO_MAP__
#define __BOO_MAP__

#include "BooPHF.hpp"
#include <vector>
#include <iterator>
#include <type_traits>

template <typename Iter>
class KeyIterator {
public:
    typedef KeyIterator<Iter> self_type;
    typedef typename std::iterator_traits<Iter>::value_type::first_type value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef std::forward_iterator_tag iterator_category;
    typedef int64_t difference_type;

    KeyIterator(Iter first) : begin_(first), curr_(first) {}
    KeyIterator operator++() { KeyIterator i = *this; curr_++; return i; }
    KeyIterator operator++(int) { ++curr_; return *this; }
    reference operator*() { return curr_->first; }
    pointer operator->() { return &(curr_->first); }
    bool operator==(const self_type& rhs) { return curr_ == rhs.curr_; }
    bool operator!=(const self_type& rhs) { return curr_ != rhs.curr_; }
    bool operator<(const self_type& rhs) { return curr_ < rhs.curr_; }
    bool operator<=(const self_type& rhs) { return curr_ <= rhs.curr_; }
    
private:
    Iter begin_;
    Iter curr_;
};

template <typename KeyT, typename ValueT>
class BooMap {
public:
    using HasherT = boomphf::SingleHashFunctor<KeyT>;
    using BooPHFT = boomphf::mphf<KeyT, HasherT>;
    using IteratorT = typename std::vector<std::pair<KeyT, ValueT>>::iterator;

    BooMap() : built_(false) {}
    void add(KeyT&& k, ValueT&& v) {
        data_.emplace_back(k, v);
    }

    bool build(int nthreads=1) {
        size_t numElem = data_.size();
        KeyIterator<decltype(data_.begin())> kb(data_.begin());
        KeyIterator<decltype(data_.begin())> ke(data_.end());
        auto keyIt = boomphf::range(kb, ke);
        BooPHFT* ph = new BooPHFT(numElem, keyIt, nthreads);
        boophf_.reset(ph);
        std::cerr << "reordering keys and values to coincide with phf";
        std::vector<size_t> inds; inds.reserve(data_.size());
        for (size_t i = 0; i < data_.size(); ++i) {
            inds.push_back(ph->lookup(data_[i].first));
        }
        reorder_destructive_(inds.begin(), inds.end(), data_.begin());
        built_ = true;
        return built_;
    }

    IteratorT find(const KeyT& k) {
        auto ind = boophf_->lookup(k);
        return (ind < data_.size()) ? (data_[ind].first == k ? data_.begin() + ind : data_.end()) : data_.end();
    }
    
    inline IteratorT begin() { return data_.begin(); }
    inline IteratorT end() { return data_.end(); }
    inline IteratorT cend() const { return data_.cend(); }
    inline IteratorT cbegin() const { return data_.cbegin(); }
    
private:
    // From : http://stackoverflow.com/questions/838384/reorder-vector-using-a-vector-of-indices
    template< typename order_iterator, typename value_iterator >
    void reorder_destructive_( order_iterator order_begin, order_iterator order_end, value_iterator v )  {
        using value_t = typename std::iterator_traits< value_iterator >::value_type;
        using index_t = typename std::iterator_traits< order_iterator >::value_type;
        using diff_t = typename std::iterator_traits< order_iterator >::difference_type;

        diff_t remaining = order_end - 1 - order_begin;
        for ( index_t s = index_t(); remaining > 0; ++ s ) {
            index_t d = order_begin[s];
            if ( d == (diff_t) -1 ) continue;
            -- remaining;
            value_t temp = v[s];
            for ( index_t d2; d != s; d = d2 ) {
                std::swap( temp, v[d] );
                std::swap( order_begin[d], d2 = (diff_t) -1 );
                -- remaining;
            }
            v[s] = temp;
        }
    }

    bool built_;
    std::vector<std::pair<KeyT, ValueT>> data_;
    std::unique_ptr<BooPHFT> boophf_{nullptr};
};
#endif // __BOO_MAP__ 
