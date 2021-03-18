/*
Copyright (c) 2020 Naomasa Matsubayashi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef IFM_2OP_H
#define IFM_2OP_H
#include <array>
#include <vector>
#include <utility>
#include <tuple>
#include <wasserstein.h>
#include <ifm/adam.h>
namespace ifm {
  constexpr int max_harmony = 50;
  std::tuple< float, float > lossimage(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre,
    unsigned int freq,
    float b,
    const std::array< int, max_harmony > &n
  );
  void generate_bessel(
    float b,
    const std::vector< std::pair< float, float > > &pre,
    std::array< float, max_harmony + 3 > &bessel
  );
  void forward_2op_nofb(
    float w,
    const std::array< float, max_harmony + 3 > &bessel,
    std::vector< float > &generated_freqs,
    std::vector< float > &generated_weights,
    std::vector< float > &n
  );
  std::tuple< float, float > backward_2op_nofb(
    float w,
    const std::array< float, max_harmony + 3 > &bessel,
    const std::vector< float > &n,
    const std::vector< float > &generated_freqs_grad,
    const std::vector< float > &generated_weights_grad
  );
  class optimize_2op_nofb {
  public:
    optimize_2op_nofb(
      std::vector< std::pair< float, float > > &&pre_,
      unsigned int thres_
    ) : w( 5 ), b( 5 ), pre( std::move( pre_ ) ), thres( thres_ ) {}
    std::tuple< float, float > operator()(
      const std::vector< float > &expected_freqs_,
      const std::vector< float > &expected_weights_
    );
    float operator()(
      const std::vector< float > &expected_freqs_,
      const std::vector< float > &expected_weights_,
      float w_
    );
    float operator()(
      const std::vector< float > &expected_freqs_,
      const std::vector< float > &expected_weights_,
      float w_,
      float b_
    );
  private:
    float w;
    float b;
    std::vector< std::pair< float, float > > pre;
    wasserstein::wasserstein_t< float > wass;
    std::array< float, max_harmony + 3 > bessel;
    std::vector< float > generated_freqs;
    std::vector< float > generated_weights;
    std::vector< float > n;
    unsigned int thres;
    //std::vector< float > generated_freqs_grad;
    //std::vector< float > generated_weights_grad;
  };
  std::tuple< float, unsigned int, float > find_b_2op(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre
  );
  std::array< int, max_harmony > generate_n(
    unsigned int freq
  );
  std::tuple< float, float > find_b_2op(
    const float *expected,
    const std::vector< std::pair< float, float > > &pre,
    unsigned int freq,
    float initial_b,
    const std::array< int, max_harmony > &n
  );
}

#endif

