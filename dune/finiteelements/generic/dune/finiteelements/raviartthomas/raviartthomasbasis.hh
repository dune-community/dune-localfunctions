// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_LAGRANGEBASIS_HH
#define DUNE_LAGRANGEBASIS_HH
#include <fstream>
#include <dune/alglib/multiprecision.hh>
#include <dune/alglib/matrix.hh>
#include <dune/grid/genericgeometry/referenceelements.hh>

#include <dune/finiteelements/lagrangepoints.hh>
#include <dune/finiteelements/lagrangeinterpolation.hh>
#include <dune/finiteelements/basisprovider.hh>
#include <dune/finiteelements/polynomialbasis.hh>
namespace Dune
{
  template <class MBasis>
  struct RaviartThomasFill
  {
    static const int dimRange = MBasis::dimension;
    RaviartThomasFill(const MBasis &basis)
      : basis_(basis)
    {}
    template <class Domain,class Iter,class Field>
    void operator()(const Domain &x, Iter iter,std::vector<Field> &vecContainer) const
    {
      typedef std::vector<Field> Container;
      typename Container::iterator vecIter = vecContainer.begin();
      unsigned int notHomogen = basis_.sizes(basis_.order())[basis_.order()-1];
      std::cout << " Order: " << basis_.order()
                << " notHomo: " << notHomogen
                << " Homo:    " << basis_.size()-notHomogen
                << std::endl;
      for (unsigned int baseFunc = 0 ;
           baseFunc<notHomogen; ++iter, ++baseFunc)
      {
        const typename Iter::Block &block = iter.block();
        for (int b=0; b<iter.blockSize; ++b)
        {
          for (int r1=0; r1<dimRange; ++r1)
          {
            for (int r2=0; r2<dimRange; ++r2)
            {
              *vecIter = (r1==r2 ? block[b] : Field(0));
              ++vecIter;
            }
          }
        }
      }
      for ( ; !iter.done(); ++iter )
      {
        const typename Iter::Block &block = iter.block();
        for (int b=0; b<iter.blockSize; ++b)
        {
          for (int r1=0; r1<dimRange; ++r1)
          {
            for (int r2=0; r2<dimRange; ++r2)
            {
              *vecIter = (r1==r2 ? block[b] : Field(0));
              ++vecIter;
            }
          }
        }
        for (int b=0; b<iter.blockSize; ++b)
        {
          for (int r1=0; r1<dimRange; ++r1)
          {
            *vecIter = x[r1]*block[b];
            ++vecIter;
          }
        }
      }
    }
    const MBasis &basis_;
  };

  template <class B>
  struct RaviartThomasEvaluator
    : public VecEvaluator<B,RaviartThomasFill<B> >
  {
    typedef RaviartThomasFill<B> Fill;
    typedef VecEvaluator< B,Fill > Base;
    static const int dimension = B::dimension;
    RaviartThomasEvaluator(const B &basis)
      : Base(basis,fill_,totalSize(basis)),
        fill_(basis)
    {}
  private:
    void totalSize(const B &basis)
    {
      unsigned int notHomogen = basis.sizes(basis.order())[basis.order()-1];
      unsigned int homogen = basis_.size()-notHomogen;
      return notHomogen*dimension+homogen*(dimension+1);
    }
    Fill fill_;
  };

  template< class Topology, class F >
  class RaviartThomasInterpolation
  {
    typedef RaviartThomasInterpolation< Topology, F > This;

  public:
    typedef F Field;

    static const unsigned int dimension = Topology::dimension;

  private:
    LagrangePoints< Topology, F > lagrangePoints_;

  public:
    RaviartThomasInterpolation ( const unsigned int order )
      : lagrangePoints_( order+2 )
    {}

    template< class Eval, class Matrix >
    void interpolate ( Eval &eval, Matrix &coefficients )
    {
      typedef typename LagrangePoints< Topology, F >::iterator Iterator;

      coefficients.resize( eval.size(), eval.size( ) );

      unsigned int row = 0;
      const Iterator end = lagrangePoints_.end();
      for( Iterator it = lagrangePoints_.begin(); it != end; ++it )
      {
        if (it->localKey().codim()>1)
          continue;
        else if (it->localKey().codim()==1)
          fillBnd( row, it->localKey(), eval.evaluate(it->point()),
                   coefficients );
        else
          for (int d=0; d<dimension; ++d)
            fillInterior( row, it->localKey(), eval.evaluate(it->point()),
                          coefficients );
      }
    }
    template <class LocalKey, class Iterator,class Matrix>
    void fillBnd(unsigned int &row,
                 const LocalKey &key, Iterator iter, Matrix &matrix) const
    {
      const Dune::FieldVector<double,dimension> &normal = GenericGeometry::ReferenceElement<Topology,double>::integrationOuterNormal(key.subEntity());
      unsigned int col = 0;
      for ( ; !iter.done() ; ++iter,++col) {
        matrix(row,col) = 0.;
        for (int d=0; d<dimension; ++d)
          matrix(row,col) += iter.block()[d]*normal[d];
      }
      ++row;
    }
    template <class LocalKey, class Iterator,class Matrix>
    void fillInterior(unsigned int &row,
                      const LocalKey &key, Iterator iter,Matrix &matrix) const
    {
      unsigned int col = 0;
      for ( ; !iter.done() ; ++iter,++col)
        for (int d=0; d<dimension; ++d)
          matrix(row+d,col) = iter.block()[d];
      row+=dimension;
    }
  };


  template <class Topology,class scalar_t>
  struct RaviartThomasMatrix {
    enum {dim = Topology::dimension};
    typedef Dune::AlgLib::Matrix< scalar_t > mat_t;
    typedef StandardMonomialBasis<dim,scalar_t> MBasis;
    RaviartThomasMatrix(int order) : basis_(order), order_(order)
    {
      typedef Dune::MonomialBasis< Topology, scalar_t > MBasis;
      MBasis basis(order);
      RaviartThomasEvaluator< MBasis > eval(basis);
      RaviartThomasInterpolation< Topology, scalar_t  > interpolation(order);
      interpolation.interpolate( eval, matrix_ );
      matrix_.invert();
    }
    int colSize(int row) const {
      return (dim+1)*basis_.size()-basis_.sizes(order_)[order_-1];
    }
    int rowSize() const {
      return (dim+1)*basis_.size()-basis_.sizes(order_)[order_-1];
    }
    const scalar_t operator() ( int r, int c ) const
    {
      return matrix_(c,r);
      // return ( (r==c)? scalar_t(1):scalar_t(0) );
    }
    void print(std::ostream& out,int N = rowSize()) const {}
    MBasis basis_;
    int order_;
    mat_t matrix_;
  };

  template< int dim, class SF, class CF >
  struct RaviartThomasBasisCreator
  {
    typedef StandardMonomialBasis<dim,SF> MBasis;
    typedef SF StorageField;
    typedef AlgLib::MultiPrecision< Precision<CF>::value > ComputeField;
    static const int dimension = dim;
    typedef PolynomialBasisWithMatrix<RaviartThomasEvaluator<MBasis>,SparseCoeffMatrix<StorageField> > Basis;
    typedef unsigned int Key;

    template <class Topology>
    struct Maker
    {
      static void apply(unsigned int order,Basis* &basis)
      {
        // bool RTBasis_only_for_implemented_for_simplex = GenericGeometry::IsSimplex<Topology>::value ;
        // assert(RTBasis_only_for_implemented_for_simplex);
        static MBasis _mBasis(order);
        basis = new Basis(_mBasis);
        RaviartThomasMatrix<Topology,ComputeField> matrix(order);
        basis->fill(matrix);
        std::stringstream name;
        name << "lagrange_" << Topology::name() << "_p" << order;
        basis->template printBasis<Topology>(name.str(),matrix);
      }
    };
  };

  template< int dim, class SF, class CF = typename ComputeField< SF, 512 >::Type >
  struct RaviartThomasBasisProvider
    : public BasisProvider<RaviartThomasBasisCreator<dim,SF,CF> >
  {};
}
#endif // DUNE_ORTHONORMALBASIS_HH
