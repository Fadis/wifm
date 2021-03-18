#ifndef WASSERSTEIN_H
#define WASSERSTEIN_H
#include <algorithm>
#include <numeric> // std::std::iota
#include <vector>
#include <boost/container/flat_map.hpp>
#include <stamp/setter.hpp>

namespace wasserstein {

template< typename T >
struct wasserstein_elem_t {
  wasserstein_elem_t() : index( 0 ), weight( 0 ), sum( 0 ) {}
  LIBSTAMP_SETTER( index )
  LIBSTAMP_SETTER( weight )
  LIBSTAMP_SETTER( sum )
  size_t index;
  T weight;
  T sum;
};

template< typename I, typename V, typename W >
void generate_indexed(
  I &indexed,
  const V &values,
  const W &weights
) {
  indexed.clear();
  using we_t = wasserstein_elem_t< typename I::key_type >;
  indexed.reserve( values.size() );
  size_t i = 0;
  for( auto v: values ) {
    indexed.insert( std::make_pair( v, we_t().set_index( i ).set_weight( weights[ i ] ) ) );
    ++i;
  }
  typename I::key_type sum = 0;
  for( auto &v: indexed ) {
    typename I::key_type cur = v.second.weight;
    v.second.set_sum( cur + sum );
    sum += cur;
  }
}

template< typename C, typename A, typename I >
void
generate_cdf_index(
  C &cdf_index,
  const A &all,
  const I &indexed
) {
  cdf_index.clear();
  cdf_index.reserve( all.size() - 1 );
  for( auto elem = all.begin(); elem != std::prev( all.end(), 1 ); ++elem ) {
    auto up = indexed.upper_bound( *elem );
    if( up == indexed.end()) cdf_index.push_back( indexed.size() );
    else cdf_index.push_back( std::distance( indexed.begin(), up ) );
  }
}

template< typename D, typename A, typename I, typename C >
void
generate_cdf(
  D &cdf,
  const A &all,
  const I &indexed,
  const C &cdf_index
) {
  cdf.clear();
  cdf.reserve( all.size() );
  typename I::key_type accum = std::prev( indexed.end() )->second.sum;
  for (auto cIdx : cdf_index ) {
    if( cIdx ) cdf.push_back( std::next( indexed.begin(), cIdx-1 )->second.sum / accum);
    else cdf.push_back( 0 );
  }
}

template < typename D, typename A >
void generate_delta(
  D &deltas,
  const A &all
) {
  deltas.clear();
  deltas.reserve( all.size() );
  for( size_t i = 0; i < all.size() - 1; ++i )
    deltas.push_back( all[ i + 1 ] - all[ i ] );
  deltas.push_back( 0 );
}

template< typename T >
struct wasserstein_resources_t {
  using we_t = wasserstein_elem_t< T >;
  std::vector<T> all;
  boost::container::flat_multimap< T, we_t > indexed_a;
  boost::container::flat_multimap< T, we_t > indexed_b;
  std::vector< size_t > cdfIdxA;
  std::vector< size_t > cdfIdxB;
  std::vector<T> cdfA;
  std::vector<T> cdfB;
  std::vector<T> deltas;
};

template <typename T>
class wasserstein_t {
  using we_t = wasserstein_elem_t< T >;
public:
  T forward(
    const std::vector<T> &A,
    const std::vector<T> &AWeights,
    const std::vector<T> &B,
    const std::vector<T> &BWeights
  ) {
    all.clear();
    all.reserve( A.size() + B.size() );
    all.insert(all.end(), A.begin(), A.end());
    all.insert(all.end(), B.begin(), B.end());
    std::sort(all.begin(), all.end());
    generate_indexed( indexed_a, A, AWeights );
    generate_indexed( indexed_b, B, BWeights );
    generate_cdf_index( cdfIdxA, all, indexed_a );
    generate_cdf_index( cdfIdxB, all, indexed_b );
    generate_cdf( cdfA, all, indexed_a, cdfIdxA );
    generate_cdf( cdfB, all, indexed_b, cdfIdxB );
    generate_delta( deltas, all );
    T dist = 0.0;
    for( size_t i = 0; i < deltas.size() - 1; ++i ) {
      dist += std::abs( cdfA[ i ] - cdfB[ i ] ) * deltas[ i ];
    }
    return dist;
  }
  static T forward_x(
    const std::vector<T> &A,
    const std::vector<T> &AWeights,
    const std::vector<T> &B,
    const std::vector<T> &BWeights
  ) {
    wasserstein_resources_t< T > res;
    res.all.reserve( A.size() + B.size() );
    res.all.insert(res.all.end(), A.begin(), A.end());
    res.all.insert(res.all.end(), B.begin(), B.end());
    std::sort(res.all.begin(), res.all.end());
    generate_indexed( res.indexed_a, A, AWeights );
    generate_indexed( res.indexed_b, B, BWeights );
    generate_cdf_index( res.cdfIdxA, res.all, res.indexed_a );
    generate_cdf_index( res.cdfIdxB, res.all, res.indexed_b );
    generate_cdf( res.cdfA, res.all, res.indexed_a, res.cdfIdxA );
    generate_cdf( res.cdfB, res.all, res.indexed_b, res.cdfIdxB );
    generate_delta( res.deltas, res.all );
    T dist = 0.0;
    for( size_t i = 0; i < res.deltas.size() - 1; ++i ) {
      dist += std::abs( res.cdfA[ i ] - res.cdfB[ i ] ) * res.deltas[ i ];
    }
    return dist;
  }
  std::tuple< std::vector< T >, std::vector< T > >
  backward(
    const std::vector<T> &A,
    const std::vector<T> &AWeights,
    const std::vector<T> &B,
    const std::vector<T> &BWeights,
    T dist,
    T delta,
    T wdelta
  ) {
    std::vector<T> dAWeights( AWeights.size() );
    std::vector<T> dA( A.size() );
    {
#pragma omp parallel for
      for( size_t i = 0; i < A.size(); ++i ) {
        auto modif_ = A;
        modif_[ i ] += delta;
        auto modified_dist = forward_x( modif_, AWeights, B, BWeights );
        dA[ i ] = ( modified_dist - dist ) / delta;
      }
    }
    {
      std::vector<T> modif = AWeights;
#pragma omp parallel for
      for( size_t i = 0; i < AWeights.size(); ++i ) {
        auto modif_ = AWeights;
        modif_[ i ] +=  wdelta;
        auto modified_dist = forward_x( A, modif_, B, BWeights );
        dAWeights[ i ] = ( modified_dist - dist ) / wdelta;
      }
    }
    return std::make_tuple( dA, dAWeights );
  }
  std::vector< T >
  backward_partial(
    const std::vector<T> &A,
    const std::vector<T> &AWeights,
    const std::vector<T> &B,
    const std::vector<T> &BWeights,
    T dist,
    T wdelta
  ) {
    std::vector<T> dAWeights( AWeights.size() );
    {
#pragma omp parallel for
      for( size_t i = 0; i < AWeights.size(); ++i ) {
        auto modif_ = AWeights;
        modif_[ i ] +=  wdelta;
        auto modified_dist = forward_x( A, modif_, B, BWeights );
        if( std::isnan( modified_dist ) ) {
          std::cout << "=== begin" << std::endl;
          for( auto v: A ) std::cout << v << " ";
          std::cout << std::endl;
          for( auto v: modif_ ) std::cout << v << " ";
          std::cout << std::endl;
          for( auto v: AWeights ) std::cout << v << " ";
          std::cout << std::endl;
          for( auto v: B ) std::cout << v << " ";
          std::cout << std::endl;
          for( auto v: BWeights ) std::cout << v << " ";
          std::cout << std::endl;
          std::cout << "=== end" << std::endl;
        }
        dAWeights[ i ] = ( modified_dist - dist ) / wdelta;
      }
    }
    return dAWeights;
  }
private:
  std::vector<T> all;
  boost::container::flat_multimap< T, we_t > indexed_a;
  boost::container::flat_multimap< T, we_t > indexed_b;
  std::vector< size_t > cdfIdxA;
  std::vector< size_t > cdfIdxB;
  std::vector<T> cdfA;
  std::vector<T> cdfB;
  std::vector<T> deltas;
};
}
#endif

