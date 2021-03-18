#include <random>
#include <iostream>
#include <vector>
#include <wasserstein.h>

int main() {
  /*std::vector< double > av( {0,3,0,0,0,1,0,0,0,0,0,1,0} );
  std::vector< double > aw( {2,4,6,8,10,12,14,16,18,20,22,24,26} );
  const std::vector< double > bv( {0,2,1,0,0,1,0,0,1,0,5,0,0} );
  const std::vector< double > bw( {2,4,6,8,10,12,14,16,18,20,22,24,26} );
*/

  std::random_device seed_gen;
  std::mt19937 engine(seed_gen());
  std::uniform_real_distribution< double > dist( 0.0, 128.0 );
  double arange = dist( engine ) / 10;
  std::vector< double > av;
  for( size_t i = 0u; i != 10u; ++i ) av.push_back( arange * i );
  std::vector< double > aw;
  for( size_t i = 0u; i != 10u; ++i ) aw.push_back( dist( engine ) );
  std::vector< double > bv;
  double brange = dist( engine ) / 10;
  for( size_t i = 0u; i != 10u; ++i ) bv.push_back( brange * i );
  std::vector< double > bw;
  for( size_t i = 0u; i != 10u; ++i ) bw.push_back( dist( engine ) );
  boost::container::flat_map< double, double > bm;
  boost::container::flat_map< double, double > am;
  for( size_t i = 0u; i != 10u; ++i )
    bm[ bv[ i ] ] = bw[ i ];

  auto bscale =  std::accumulate( bw.begin(), bw.end(), 0.0 ) ;
  auto scale = std::accumulate( bw.begin(), bw.end(), 0.0 ) / std::accumulate( aw.begin(), aw.end(), 0.0 );
  for( auto &v: aw ) v *= scale;

  wasserstein::wasserstein_t< double > wass;
  size_t iter = 0;
  while( 1 ) {
    auto dist = wass.forward(av,aw,bv,bw);
    //std::cout << dist << std::endl;
    if( dist < 0.00005 ) break;
    auto [dav,daw] = wass.backward(av,aw,bv,bw,dist,0.0001f,0.0001f);
    size_t i = 0;
    double darange = 0.0;
    for( auto v: dav ) {
      darange += dist * 0.001 * v * i;
      ++i;
    }
    arange -= darange;
    //std::cout << arange << " " << brange << " " << darange << std::endl;
    av.clear();
    for( size_t i = 0u; i != 10u; ++i ) av.push_back( arange * i );
    for( size_t i = 0; i != aw.size(); ++i ) aw[ i ] -= dist * 0.001 * daw[ i ];
    auto scale = bscale / std::accumulate( aw.begin(), aw.end(), 0.0 );
    for( auto &v: aw ) v *= scale;
    ++iter;
    if( !( iter % 100000 ) ) {
      am.clear();
      for( size_t i = 0u; i != 10u; ++i )
        am[ av[ i ] ] = aw[ i ];
      std::cout << "======== " << dist << " ========"<< std::endl;
      for( auto v: bm ) std::cout << v.first << " ";
      std::cout << std::endl;
      for( auto v: am ) std::cout << v.first << " ";
      std::cout << std::endl;
      for( auto v: bm ) std::cout << v.second << " ";
      std::cout << std::endl;
      for( auto v: am ) std::cout << v.second << " ";
      std::cout << std::endl;
    }
  }
  for( auto v: bw ) std::cout << v << " ";
  std::cout << std::endl;
  for( auto v: aw ) std::cout << v << " ";
  std::cout << std::endl;
}
