// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_ALGLIB_MATRIX_HH
#define DUNE_ALGLIB_MATRIX_HH

#include <cassert>
#include <vector>

#include <dune/finiteelements/common/vector.hh>
#include <dune/finiteelements/common/field.hh>

#if HAVE_ALGLIB
#include <alglib/amp.h>
#include <alglib/inv.h>
#endif

namespace Dune
{

  template< class F, bool aligned = false >
  class LFEMatrix;


  template< class F, bool aligned >
  class LFEMatrix
  {
    typedef LFEMatrix< F, aligned > This;

  public:
    typedef F Field;
    typedef std::vector< F > Vec;

  private:
    typedef std::vector<Vec> RealMatrix;

  public:
    operator const RealMatrix & () const
    {
      return matrix_;
    }

    operator RealMatrix & ()
    {
      return matrix_;
    }

    const RealMatrix &amp () const
    {
      return matrix_;
    }

    const Field &operator() ( const unsigned int row, const unsigned int col ) const
    {
      assert(row<rows());
      assert(col<cols());
      return matrix_[ row ][ col ];
    }

    Field &operator() ( const unsigned int row, const unsigned int col )
    {
      assert(row<rows());
      assert(col<cols());
      return matrix_[ row ][ col ];
    }

    unsigned int rows () const
    {
      return rows_;
    }

    unsigned int cols () const
    {
      return cols_;
    }

    const Field *rowPtr ( const unsigned int row ) const
    {
      assert(row<rows());
      return &(matrix_[row][0]);
    }

    Field *rowPtr ( const unsigned int row )
    {
      assert(row<rows());
      return &(matrix_[row][0]);
    }

    void resize ( const unsigned int rows, const unsigned int cols )
    {
      matrix_.resize(rows);
      for (unsigned int i=0; i<rows; ++i)
        matrix_[i].resize(cols);
      rows_ = rows;
      cols_ = cols;
    }

    bool invert ()
    {
      assert( rows() == cols() );
      std::vector<unsigned int> p(rows());
      for (unsigned int j=0; j<rows(); ++j)
        p[j] = j;
      for (unsigned int j=0; j<rows(); ++j)
      {
        // pivot search
        unsigned int r = j;
        Field max = std::abs( (*this)(j,j) );
        for (unsigned int i=j+1; i<rows(); ++i)
        {
          if ( std::abs( (*this)(i,j) ) > max )
          {
            max = std::abs( (*this)(i,j) );
            r = i;
          }
        }
        if (max == 0)
          abort(); // return false;
        // row swap
        if (r > j)
        {
          for (unsigned int k=0; k<rows(); ++k)
            std::swap( (*this)(j,k), (*this)(r,k) );
          std::swap( p[j], p[r] );
        }
        // transformation
        Field hr = Unity<Field>()/(*this)(j,j);
        for (unsigned int i=0; i<rows(); ++i)
          (*this)(i,j) *= hr;
        (*this)(j,j) = hr;
        for (unsigned int k=0; k<rows(); ++k)
        {
          if (k==j) continue;
          for (unsigned int i=0; i<rows(); ++i)
          {
            if (i==j) continue;
            (*this)(i,k) -= (*this)(i,j)*(*this)(j,k);
          }
          (*this)(j,k) *= -hr;
        }
      }
      // column exchange
      Vec hv(rows());
      for (unsigned int i=0; i<rows(); ++i)
      {
        for (unsigned int k=0; k<rows(); ++k)
          hv[ p[k] ] = (*this)(i,k);
        for (unsigned int k=0; k<rows(); ++k)
          (*this)(i,k) = hv[k];
      }
      return true;
    }

  private:
    RealMatrix matrix_;
    unsigned int cols_,rows_;
  };

#if HAVE_ALGLIB
  template< unsigned int precision, bool aligned >
  class LFEMatrix< amp::ampf< precision >, aligned >
  {
    typedef LFEMatrix< amp::ampf< precision >, aligned > This;

  public:
    typedef amp::ampf< precision > Field;
    typedef LFEVector< Field > Vec;

  private:
    typedef ap::template_2d_array< Field, aligned > RealMatrix;

  public:
    operator const RealMatrix & () const
    {
      return matrix_;
    }

    operator RealMatrix & ()
    {
      return matrix_;
    }

    const RealMatrix &amp () const
    {
      return matrix_;
    }

    const Field &operator() ( const unsigned int row, const unsigned int col ) const
    {
      assert(row<rows());
      assert(col<cols());
      return matrix_( row, col );
    }

    Field &operator() ( const unsigned int row, const unsigned int col )
    {
      assert(row<rows());
      assert(col<cols());
      return matrix_( row, col );
    }

    unsigned int rows () const
    {
      return matrix_.gethighbound( 1 )+1;
    }

    unsigned int cols () const
    {
      return matrix_.gethighbound( 2 )+1;
    }

    const Field *rowPtr ( const unsigned int row ) const
    {
      assert(row<rows());
      const int lastCol = matrix_.gethighbound( 2 );
      ap::const_raw_vector< Field > rowVector = matrix_.getrow( row, 0, lastCol );
      assert( (rowVector.GetStep() == 1) && (rowVector.GetLength() == lastCol+1) );
      return rowVector.GetData();
    }

    Field *rowPtr ( const unsigned int row )
    {
      assert(row<rows());
      const int lastCol = matrix_.gethighbound( 2 );
      ap::raw_vector< Field > rowVector = matrix_.getrow( row, 0, lastCol );
      assert( (rowVector.GetStep() == 1) && (rowVector.GetLength() == lastCol+1) );
      return rowVector.GetData();
    }

    void resize ( const unsigned int rows, const unsigned int cols )
    {
      matrix_.setbounds( 0, rows-1, 0, cols-1 );
    }

    bool invert ()
    {
      assert( rows() == cols() );
      return inv::rmatrixinverse< precision >( matrix_, rows() );
    }

  private:
    RealMatrix matrix_;
  };
#endif

  template< class Field, bool aligned >
  inline std::ostream &operator<<(std::ostream &out, const LFEMatrix<Field,aligned> &mat)
  {
    for (unsigned int r=0; r<mat.rows(); ++r)
    {
      out << field_cast<double>(mat(r,0));
      for (unsigned int c=1; c<mat.cols(); ++c)
      {
        out << " , " << field_cast<double>(mat(r,c));
      }
      out << std::endl;
    }
    return out;
  }
}

#endif // #ifndef DUNE_ALGLIB_MATRIX_HH