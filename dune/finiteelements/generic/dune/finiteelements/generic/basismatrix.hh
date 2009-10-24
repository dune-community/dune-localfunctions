// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_BASISMATRIX_HH
#define DUNE_BASISMATRIX_HH

#include <fstream>
#include <dune/common/exceptions.hh>

#include <dune/finiteelements/common/matrix.hh>
#include <dune/finiteelements/generic/monomialbasis.hh>
#include <dune/finiteelements/generic/polynomialbasis.hh>

namespace Dune
{
  template< class PreBasis, class Interpolation,
      class Field >
  struct BasisMatrix
  {
    typedef typename PreBasis::HALLO HALLO;
  };

  template< class PreBasis, class Interpolation,
      class Field >
  struct BasisMatrixBase : public LFEMatrix<Field>
  {
    typedef LFEMatrix<Field> Matrix;

    BasisMatrixBase( const PreBasis& preBasis,
                     const Interpolation& localInterpolation )
      : cols_(preBasis.size())
    {
      localInterpolation.interpolate( preBasis, *this );
      if ( !Matrix::invert() )
      {
        DUNE_THROW(MathError, "While computing LagrangeBasis a singular matrix was constructed!");
      }
    }
    unsigned int cols () const
    {
      return cols_;
    }
    unsigned int rows () const
    {
      return Matrix::rows();
    }
  private:
    unsigned int cols_;
  };

  template< class Topology, class F,
      class Interpolation,
      class Field >
  struct BasisMatrix< const MonomialBasis< Topology, F >, Interpolation, Field >
    : public BasisMatrixBase< const MonomialBasis< Topology, F >, Interpolation, Field >
  {
    typedef const MonomialBasis< Topology, F > PreBasis;
    typedef BasisMatrixBase<PreBasis,Interpolation,Field> Base;
    typedef typename Base::Matrix Matrix;

    BasisMatrix( const PreBasis& preBasis,
                 const Interpolation& localInterpolation )
      : Base(preBasis, localInterpolation)
    {}
    template <class Vector>
    void row( const unsigned int row, Vector &vec ) const
    {
      const unsigned int N = Matrix::rows();
      assert( Matrix::cols() == N && vec.size() == N );
      // note: that the transposed matrix is computed,
      //       and is square
      for (unsigned int i=0; i<N; ++i)
        field_cast(Matrix::operator()(i,row),vec[i]);
    }
  };
  template< int dim, class F,
      class Interpolation,
      class Field >
  struct BasisMatrix< const Dune::VirtualMonomialBasis< dim, F >, Interpolation, Field >
    : public BasisMatrixBase< const VirtualMonomialBasis< dim, F >, Interpolation, Field >
  {
    typedef const VirtualMonomialBasis< dim, F > PreBasis;
    typedef BasisMatrixBase<PreBasis,Interpolation,Field> Base;
    typedef typename Base::Matrix Matrix;

    BasisMatrix( const PreBasis& preBasis,
                 const Interpolation& localInterpolation )
      : Base(preBasis, localInterpolation)
    {}
    template <class Vector>
    void row( const unsigned int row, Vector &vec ) const
    {
      const unsigned int N = Matrix::rows();
      assert( Matrix::cols() == N && vec.size() == N );
      // note: that the transposed matrix is computed,
      //       and is square
      for (unsigned int i=0; i<N; ++i)
        field_cast(Matrix::operator()(i,row),vec[i]);
    }
  };
  template< class Eval, class CM, class D, class R,
      class Interpolation,
      class Field >
  struct BasisMatrix< const PolynomialBasis<Eval,CM,D,R>, Interpolation, Field >
    : public BasisMatrixBase< const PolynomialBasis<Eval,CM,D,R>, Interpolation, Field >
  {
    typedef const PolynomialBasis<Eval,CM,D,R> PreBasis;
    typedef BasisMatrixBase<PreBasis,Interpolation,Field> Base;
    typedef typename Base::Matrix Matrix;

    BasisMatrix( const PreBasis& preBasis,
                 const Interpolation& localInterpolation )
      : Base(preBasis, localInterpolation),
        preBasis_(preBasis)
    {}
    unsigned int cols() const
    {
      return preBasis_.matrix().baseSize() ;
    }
    template <class Vector>
    void row( const unsigned int row, Vector &vec ) const
    {
      assert( Matrix::rows() == Matrix::cols() );
      assert( vec.size() == preBasis_.matrix().baseSize() );
      assert( Matrix::cols() == preBasis_.size() );
      for (unsigned int j=0; j<Matrix::cols(); ++j)
        vec[j] = 0;
      for (unsigned int i=0; i<Matrix::rows(); ++i)
        preBasis_.matrix().
        addRow(i,Base::Matrix::operator()(i,row),vec);
    }
  private:
    const PreBasis& preBasis_;
  };
  template< class Eval, class CM,
      class Interpolation,
      class Field >
  struct BasisMatrix< const PolynomialBasisWithMatrix<Eval,CM>, Interpolation, Field >
    : public BasisMatrixBase< const PolynomialBasisWithMatrix<Eval,CM>, Interpolation, Field >
  {
    typedef const PolynomialBasisWithMatrix<Eval,CM> PreBasis;
    typedef BasisMatrixBase<PreBasis,Interpolation,Field> Base;
    typedef typename Base::Matrix Matrix;

    BasisMatrix( const PreBasis& preBasis,
                 const Interpolation& localInterpolation )
      : Base(preBasis, localInterpolation),
        preBasis_(preBasis)
    {}
    unsigned int cols() const
    {
      return preBasis_.matrix().baseSize() ;
    }
    template <class Vector>
    void row( const unsigned int row, Vector &vec ) const
    {
      assert( Matrix::rows() == Matrix::cols() );
      assert( vec.size() == preBasis_.matrix().baseSize() );
      assert( Matrix::cols() == preBasis_.size() );
      for (unsigned int j=0; j<vec.size(); ++j)
        vec[j] = 0;
      for (unsigned int i=0; i<Matrix::rows(); ++i)
        preBasis_.matrix().
        addRow(i,Base::Matrix::operator()(i,row),vec);
    }
  private:
    const PreBasis& preBasis_;
  };
}

#endif // DUNE_BASISMATRIX_HH