#include "cnn/edges.h"

#include <limits>
#include <cmath>
#include <sstream>

using namespace std;

namespace cnn {

Tensor Reshape::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  Tensor fx = *xs.front();
  assert(fx.rows() * fx.cols() == from.size());
  fx.resize(to.size(0), to.size(1));
  return fx;
}

Tensor Reshape::backward(const vector<const Tensor*>& xs,
                            const Tensor& fx,
                            const Tensor& dEdf,
                            unsigned i) const {
  Tensor dEdx = dEdf;
  int cols = from.size(1);
  if (!cols) ++cols;
  dEdx.resize(from.size(0), cols);
  return dEdx;
}

Tensor SumColumns::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  return x.rowwise().sum();
}

Tensor SumColumns::backward(const vector<const Tensor*>& xs,
                            const Tensor& fx,
                            const Tensor& dEdf,
                            unsigned i) const {
  Tensor dEdx = *xs.front();
  const int c = dEdf.cols();
  for (int j = 0; j < c; ++j)
    dEdx.col(j) = dEdf;
  return dEdx;
}

Tensor KMHNGram::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  const int new_cols = x.cols() - n + 1;
  assert(new_cols > 0);
  const int new_rows = x.rows();
  Tensor res = Zero(Dim({new_rows, new_cols}));
  for (int j = 0; j < new_cols; ++j) {
    auto c_j = res.col(j);
    for (int k = 0; k < n; ++k)
      c_j += x.col(j + k);
  }
  return res;
}

Tensor KMHNGram::backward(const vector<const Tensor*>& xs,
                          const Tensor& fx,
                          const Tensor& dEdf,
                          unsigned i) const {
  Tensor dEdx = *xs.front();
  dEdx.setZero();
  const int c = dEdf.cols();
  for (int j = 0; j < c; ++j)
    for (int k = 0; k < n; ++k)
      dEdx.col(j+k) += dEdf.col(j);
  return dEdx;
}

Tensor InnerProduct3D_1D::forward(const vector<const Tensor*>& xs) const {
  assert(!"not implemented");
}

Tensor InnerProduct3D_1D::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  assert(!"not implemented");
}

Tensor GaussianNoise::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs[0];
  return x + RandomNormal(cnn::size(x), 0, stddev);
}

Tensor GaussianNoise::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  assert(i == 0);
  return dEdf;
};

Tensor Dropout::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs[0];
  noise_mask = RandomBernoulli(cnn::size(x), p);
  return x.cwiseProduct(noise_mask);
}

Tensor Dropout::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  assert(i == 0);
  return dEdf.cwiseProduct(noise_mask);
};

Tensor OneMinusX::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs[0];
  return Constant(cnn::size(x), c) - x;
}

Tensor OneMinusX::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  return -dEdf;
};

Tensor Sum::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() > 0);
  Tensor res = *xs[0];
  for (unsigned i = 1; i < xs.size(); ++i)
    res += *xs[i];
  return res;
}

Tensor Sum::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  return dEdf;
};

Tensor Tanh::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  return Elewise::TanhForward(x);
}

Tensor Tanh::backward(const vector<const Tensor*>& xs,
                      const Tensor& fx,
                      const Tensor& dEdf,
                      unsigned i) const {
  assert(i == 0);
  const Tensor& x = *xs.front();
  return Elewise::TanhBackward(dEdf, fx, x);
}

Tensor Square::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1); // just a single input
  const Tensor& x = *xs.front();
  return x.cwiseProduct(x);
}

Tensor Square::backward(const vector<const Tensor*>& xs,
                        const Tensor& fx,
                        const Tensor& dEdf,
                        unsigned i) const {
  assert(i == 0);
  return dEdf.cwiseProduct(*xs.front()) * 2;
};

Tensor Exp::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  return Elewise::Exp(*xs.front());
}

Tensor Exp::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  return dEdf.array() * fx.array();
}

Tensor Log::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  return Elewise::Ln(*xs.front());
}

Tensor Log::backward(const vector<const Tensor*>& xs,
                     const Tensor& fx,
                     const Tensor& dEdf,
                     unsigned i) const {
  return dEdf.array() / xs[0]->array();
}

Tensor Concatenate::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() > 0);
  unsigned rows = 0;
  for (auto x : xs) rows += x->rows();
  src_row_indices.resize(xs.size());
  Tensor fx(rows, 1);
  unsigned i = 0;
  unsigned k = 0;
  for (auto x : xs) {
    src_row_indices[k] = i;
    ++k;
    const Tensor& cx = *x;
    assert(cx.cols() == 1); // this can be relaxed to the same everywhere
    const unsigned crows = cx.rows();
    for (unsigned j = 0; j < crows; ++j) {
      fx(i, 0) = cx(j, 0);
      ++i;
    }
  }
  return fx;
}

Tensor Concatenate::backward(const vector<const Tensor*>& xs,
                             const Tensor& fx,
                             const Tensor& dEdf,
                             unsigned i) const {
  assert(i < src_row_indices.size());
  Tensor dEdx = *xs[i];
  const unsigned rows = dEdx.rows();
  const unsigned begin = src_row_indices[i];
  assert(rows + begin <= dEdf.rows());
  for (unsigned i = 0; i < rows; ++i)
    dEdx(i,0) = dEdf(i + begin, 0);
  return dEdx;
}

Tensor ConcatenateColumns::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() > 0);
  const unsigned rows = xs.front()->rows();
  Tensor fx(rows, xs.size());
  unsigned i = 0;
  for (auto x : xs) {
    assert(x->rows() == rows);
    for (unsigned j = 0; j < rows; ++j)
      fx(j, i) = (*x)(j, 0);
    ++i;
  }
  return fx;
}

Tensor ConcatenateColumns::backward(const vector<const Tensor*>& xs,
                                    const Tensor& fx,
                                    const Tensor& dEdf,
                                    unsigned i) const {
  assert(i < fx.cols());
  return dEdf.col(i);
}

Tensor Hinge::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  const unsigned rows = x.rows();
  if (u.rows() != rows)
    u = Tensor(rows, 1);  // local forward value
  real y = 0;
  const real mlystar = margin - x(*pelement, 0);
  for (unsigned i = 0; i < rows; ++i)
    if (*pelement != i)
      y += u(i, 0) = max(real(0), mlystar + x(i,0));
  Tensor res(1,1);
  res(0,0) = y;
  return res;
}

Tensor Hinge::backward(const vector<const Tensor*>& xs,
                       const Tensor& fx,
                       const Tensor& dEdf,
                       unsigned i) const {
  assert(i == 0);
  const Tensor& x = *xs.front();
  const unsigned rows = x.rows();
  Tensor dEdx = Zero(Dim(rows, 1));
  if (fx(0,0) == 0) return dEdx;
  const real diff = dEdf(0,0);
  unsigned tv = 0;
  for (unsigned i = 0; i < rows; ++i)
    if (*pelement != i && u(i, 0) > 0) { dEdx(i, 0) = diff; tv++; }
  dEdx(*pelement, 0) = -diff * tv;
  return dEdx;
}

Tensor Identity::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  return *xs.front();
}

Tensor Identity::backward(const vector<const Tensor*>& xs,
                  const Tensor& fx,
                  const Tensor& dEdf,
                  unsigned i) const {
  assert(i == 0);
  return dEdf;
}

Tensor MaxPooling1D::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  const unsigned x_rows = x.rows();
  assert(x.cols() == 1);
  const unsigned fx_rows = x_rows / width;
  ind.resize(fx_rows);
  Tensor fx = Zero(Dim(fx_rows, 1));
  for (unsigned i = 0; i < fx_rows; ++i) {
    unsigned from = i * width;
    unsigned to = from + width;
    if (to > x_rows) to = x_rows;
    real best = x(from, 0);
    unsigned bestr = from;
    for (unsigned r = from + 1; r < to; ++r) {
      if (x(r, 0) > best) {
        best = x(r,0);
        bestr = r;
      }
    }
    ind[i] = bestr;
    fx(i, 0) = best;
  }
  return fx;
}

Tensor MaxPooling1D::backward(const vector<const Tensor*>& xs,
                  const Tensor& fx,
                  const Tensor& dEdf,
                  unsigned i) const {
  const Tensor& x = *xs.front();
  const unsigned x_rows = x.rows();
  Tensor dEdx = Zero(Dim(x_rows, 1));
  const unsigned fx_rows = x_rows / width;
  assert(fx_rows == ind.size());
  assert(fx_rows == dEdf.rows());
  for (unsigned i = 0; i < fx_rows; ++i)
    dEdx(ind[i], 0) = dEdf(i, 0);
  return dEdx;
}

Tensor Softmax::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  return Convolution::SoftmaxForward(x, 1);
}

Tensor Softmax::backward(const vector<const Tensor*>& xs,
                            const Tensor& fx,
                            const Tensor& dEdf,
                            unsigned i) const {
  assert(i == 0);
  return Convolution::SoftmaxBackward(dEdf, fx, 1);
}

Tensor PickNegLogSoftmax::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  assert(x.cols() == 1); // need to generalize for multiple vectors
  v = Convolution::SoftmaxForward(x, 1);
  float cll = -log(v(*pval, 0));
  return Constant({1}, cll);
}

Tensor PickNegLogSoftmax::backward(const vector<const Tensor*>& xs,
                            const Tensor& fx,
                            const Tensor& dEdf,
                            unsigned i) const {
  assert(i == 0);
  float r = dEdf(*pval, 0) / v(*pval, 0);
  // TODO finish implementing
  cerr << "not implemented\n";
  abort();
  //return Convolution::SoftmaxBackward(dEdf.cwiseQuotient(v), v, 1);
}

Tensor LogSoftmax::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  return Convolution::SoftmaxForward(x, 1).array().log();
}

Tensor LogSoftmax::backward(const vector<const Tensor*>& xs,
                            const Tensor& fx,
                            const Tensor& dEdf,
                            unsigned i) const {
  assert(i == 0);
  Tensor u = fx.array().exp();
  return Convolution::SoftmaxBackward(dEdf.cwiseQuotient(u), u, 1);
}

inline real logsumexp(const Tensor& x, const vector<unsigned>& denom) {
  real m = x(denom[0],0);
  for (auto i : denom) {
    real r = x(i,0);
    if (r > m) m = r;
  }
  real z = 0;
  for (auto i : denom)
    z += expf(x(i,0) - m);
  return m + logf(z);
}

Tensor RestrictedLogSoftmax::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  assert(denom.size() > 0);
  const Tensor& x = *xs.front();
  const unsigned rows = x.rows();
  assert(x.cols() == 1);
  const real logz = logsumexp(x, denom);
  Tensor fx(rows, 1);
  for (unsigned i = 0; i < rows; ++i)
    fx(i,0) = -numeric_limits<real>::infinity();
  for (auto i : denom)
    fx(i,0) = x(i,0) - logz;
  if (denom.size() == 1) fx(denom.front(), 0) = 0;
  return fx;
}

Tensor RestrictedLogSoftmax::backward(const vector<const Tensor*>& xs,
                            const Tensor& fx,
                            const Tensor& dEdf,
                            unsigned i) const {
  assert(i == 0);
  const Tensor& x = *xs.front();
  const unsigned rows = x.rows();
  Tensor dEdx = Zero(Dim(rows, 1));
  double z = 0;
  for (auto i : denom)
    z += dEdf(i, 0);
  for (auto i : denom)
    dEdx(i, 0) = dEdf(i, 0) - exp(fx(i, 0)) * z;
  return dEdx;
}

// x_1 is a vector
// y = (x_1)_{*pval}
Tensor PickElement::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  assert(x.cols() == 1);
  assert(*pval < x.rows());
  Tensor fx(1,1);
  fx(0,0) = x(*pval, 0);
  return fx;
}

// derivative is 0 in all dimensions except 1 for the selected element
Tensor PickElement::backward(const vector<const Tensor*>& xs,
                    const Tensor& fx,
                    const Tensor& dEdf,
                    unsigned i) const {
  assert(i == 0);
  assert(dEdf.rows() == 1);
  assert(dEdf.cols() == 1);
  const Tensor& x = *xs.front();

  // TODO should be sparse
  Tensor dEdx = Zero(Dim(x.rows(), 1)); 
  dEdx(*pval,0) = dEdf(0,0);
  return dEdx;
}

// x_1 is a vector
// y = (x_1)[start:end]
// slice of vector from index start (inclusive) to index end (exclusive)
Tensor PickRange::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  assert(x.cols() == 1);
  assert(start >= 0);
  assert(end <= x.rows());
  assert(start < end);
  Tensor fx = x.block(start, 0, end-start, 1);
  assert(fx.rows() == end-start);
  return fx;
}

// derivative is 0 in all dimensions except the slice range
Tensor PickRange::backward(const vector<const Tensor*>& xs,
                    const Tensor& fx,
                    const Tensor& dEdf,
                    unsigned i) const {
  assert(i == 0);
  assert(dEdf.rows() == end-start);
  assert(dEdf.cols() == 1);
  const Tensor& x = *xs.front();

  // TODO should be sparse
  Tensor dEdx = Tensor::Zero(x.rows(), 1);
  dEdx.block(start, 0, end-start, 1) = dEdf;
  return dEdx;
}

Tensor MatrixMultiply::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 2);
  return (*xs[0]) * (*xs[1]);
}

Tensor MatrixMultiply::backward(const vector<const Tensor*>& xs,
                                const Tensor& fx,
                                const Tensor& dEdf,
                                unsigned i) const {
  assert(i < 2);
  if (i == 0) {
    return dEdf * xs[1]->transpose();
  } else {
    return xs[0]->transpose() * dEdf;
  }
}

Tensor CwiseMultiply::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 2);
  return xs[0]->cwiseProduct(*xs[1]);
}

Tensor CwiseMultiply::backward(const vector<const Tensor*>& xs,
                               const Tensor& fx,
                               const Tensor& dEdf,
                               unsigned i) const {
  assert(i < 2);
  if (i == 0) {
    return dEdf.cwiseProduct(*xs[1]);
  } else {
    return dEdf.cwiseProduct(*xs[0]);
  }
}

Tensor Multilinear::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() % 2 == 1);
  Tensor fx = *xs.front();
  //cerr << "Multilinear\n";
  //for (unsigned i = 0; i < xs.size(); i++)
  //  cerr << " (" << xs[i]->rows() << "," << xs[i]->cols() << ")\n";
  for (unsigned i = 1; i < xs.size(); i += 2) {
    if (xs[i]->cols() == 1 && xs[i+1]->cols() == 1)
      fx += xs[i]->cwiseProduct(*xs[i + 1]);
    else
      fx += (*xs[i]) * (*xs[i + 1]);
  }
  return fx;
}

Tensor Multilinear::backward(const vector<const Tensor*>& xs,
                             const Tensor& fx,
                             const Tensor& dEdf,
                             unsigned i) const {
  assert(i < xs.size());
  if (i == 0) return dEdf;
  if (i % 2 == 1) {  // is a matrix
    if (xs[i]->cols() == 1)  // diagonal matrix
      return dEdf.cwiseProduct(*xs[i+1]);
    else
      return dEdf * xs[i+1]->transpose();
  }
  // is a vector
  if (xs[i-1]->cols() == 1)  // xs[i-1] is a diagonal matrix
    return xs[i-1]->cwiseProduct(dEdf);
  return xs[i-1]->transpose() * dEdf;
}

Tensor Negate::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  return -(*xs[0]);
}

Tensor Negate::backward(const vector<const Tensor*>& xs,
                        const Tensor& fx,
                        const Tensor& dEdf,
                        unsigned i) const {
  assert(i == 0);
  return -dEdf;
}

Tensor Rectify::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  return Elewise::ReluForward(*xs.front());
}

Tensor Rectify::backward(const vector<const Tensor*>& xs,
                         const Tensor& fx,
                         const Tensor& dEdf,
                         unsigned i) const {
  return Elewise::ReluBackward(dEdf, fx, *xs.front());
}

Tensor SquaredEuclideanDistance::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 2);
  Tensor res(1,1);
  res(0,0) = (*xs[0] - *xs[1]).squaredNorm();
  return res;
}

Tensor SquaredEuclideanDistance::backward(const vector<const Tensor*>& xs,
                                 const Tensor& fx,
                                 const Tensor& dEdf,
                                 unsigned i) const {
  assert(i < 2);
  real scale = dEdf(0,0) * 2;
  if (i == 1) scale = -scale;
  return scale * (*xs[0] - *xs[1]);
}

Tensor LogisticSigmoid::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  const Tensor& x = *xs.front();
  return Elewise::SigmoidForward(x);
}

Tensor LogisticSigmoid::backward(const vector<const Tensor*>& xs,
                                 const Tensor& fx,
                                 const Tensor& dEdf,
                                 unsigned i) const {
  assert(i == 0);
  const Tensor& x = *xs.front();
  return Elewise::SigmoidBackward(dEdf, fx, x);
}

// you could do this with LogisticSigmoid, Softmax or a variety of other
// functions, but this is often useful.
// x_1 must be a scalar that is a value between 0 and 1
// target_y is a value between 0 and 1
// y = ty * log(x_1) + (1 - ty) * log(x_1)
Tensor BinaryLogLoss::forward(const vector<const Tensor*>& xs) const {
  assert(xs.size() == 1);
  assert(xs.front()->cols() == 1);
  assert(xs.front()->rows() == 1);
  const real y_pred = (*xs.front())(0,0);
  assert(y_pred >= 0.);
  assert(y_pred <= 1.);
  const real ty = *ptarget_y;
  assert(ty >= 0.);
  assert(ty <= 1.);
  Tensor fx = *xs.front();
  real& res = fx(0,0);
  res = 0;
  if (ty > 0.) res -= ty * log(y_pred);
  if ((1 - ty) > 0.) res -= (1 - ty) * log1p(-y_pred);
  return fx;
}

Tensor BinaryLogLoss::backward(const vector<const Tensor*>& xs,
                  const Tensor& fx,
                  const Tensor& dEdf,
                  unsigned i) const {
  const real y_pred = (*xs.front())(0,0);
  const real ty = *ptarget_y;
  real scale = 0;
  if (ty > 0.) scale -= ty / y_pred;
  if ((1 - ty) >= 0.) scale += (1 - ty) / (1 - y_pred);
  return dEdf * scale;
}

} // namespace cnn