// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_MONOMIALBASIS_HH
#define DUNE_MONOMIALBASIS_HH

#include <vector>

#include <dune/common/fvector.hh>

#include <dune/grid/genericgeometry/topologytypes.hh>

namespace Dune
{

  template <class Field>
  struct Unity {
    operator Field() const {
      return Field(1);
    }
  };
  template <class Field>
  struct Zero {
    operator Field() const {
      return Field(0);
    }
  };
  template <class Field>
  bool operator<(const Zero<Field>& ,const Field& f) {
    return f>1e-12;
  }
  template <class Field>
  bool operator<(const Field& f, const Zero<Field>&) {
    return f<1e-12;
  }
  template <class Field>
  bool operator>(const Zero<Field>& z,const Field& f) {
    return f<z;
  }
  template <class Field>
  bool operator>(const Field& f, const Zero<Field>& z) {
    return z<f;
  }

  // MultiIndex class for monomial basis
  template <int dim>
  struct MultiIndex {
    static const int dimension=dim;
    MultiIndex( ) {
      vecZ_   = Vector(0);
      vecOMZ_ = Vector(0);
    }
    void set(int d) {
      vecZ_[d] = 1;
    }
    MultiIndex(int ,const MultiIndex& other)
      : vecZ_(other.vecOMZ_), vecOMZ_(other.vecZ_)
    {}
    MultiIndex(const MultiIndex& other)
      : vecZ_(other.vecZ_), vecOMZ_(other.vecOMZ_)
    {}
    MultiIndex& operator=(const MultiIndex& other) {
      vecZ_   = other.vecZ_;
      vecOMZ_ = other.vecOMZ_;
      return *this;
    }
    MultiIndex& operator*=(const MultiIndex& other) {
      vecZ_   += other.vecZ_;
      vecOMZ_ += other.vecOMZ_;
      return *this;
    }
    MultiIndex& operator/=(const MultiIndex& other) {
      vecZ_   -= other.vecZ_;
      vecOMZ_ -= other.vecOMZ_;
      return *this;
    }
    MultiIndex operator* (const MultiIndex& b) const {
      MultiIndex z = *this;
      return (z*=b);
    }
    MultiIndex operator/ (const MultiIndex& b) const {
      MultiIndex z = *this;
      return (z/=b);
    }
    int absZ() const {
      int ret=0;
      for (int i=0; i<dim; ++i)
        ret+=vecZ_[i];
      return ret;
    }
    int absOMZ() const {
      int ret=0;
      for (int i=0; i<dim; ++i)
        ret+=std::abs(vecOMZ_[i]);
      return ret;
    }
    typedef Dune::FieldVector<int,dim> Vector;
    Vector vecZ_;
    Vector vecOMZ_;
  };
  template <int d>
  std::ostream &operator<<(std::ostream& out,const MultiIndex<d>& mi) {
    if (mi.absZ()==0)
      out << "1";
    else {
      int absVal = 0;
      for (int i=0; i<d; ++i) {
        if (mi.vecZ_[i]==0)
          continue;
        else if (mi.vecZ_[i]==1)
          out << char('a'+i);
        else if (mi.vecZ_[i]>0)
          out << char('a'+i) << "**(" << mi.vecZ_[i] << ")";
        else if (mi.vecZ_[i]<0)
          out << char('a'+i) << "**(" << mi.vecZ_[i] << ")";
        absVal += mi.vecZ_[i];
        if (absVal<mi.absZ()) out << "*";
      }
    }
    if (mi.absOMZ()==0)
      out << "";
    else {
      for (int i=0; i<=mi.absZ(); ++i) {
        if (mi.vecOMZ_[i]==0)
          continue;
        else if (mi.vecOMZ_[i]==1)
          out << (1-char('a'+i));
        else if (mi.vecOMZ_[i]>0)
          out << (1-char('a'+i)) << "^" << mi.vecOMZ_[i];
        else if (mi.vecOMZ_[i]<0)
          out << (1-char('a'+i)) << "^" << mi.vecOMZ_[i];
        if (i==mi.absZ()+1) out << "*";
      }
    }
    return out;
  }
  template <int d>
  struct Unity<MultiIndex<d> > {
    typedef MultiIndex<d> Field;
    operator Field () const
    {
      return Field();
    }
    Field operator- (const Field& other) const {
      return Field(1,other);
    }
    Field operator/ (const Field& other) const {
      return Field()/other;
    }
  };
  template <int d>
  struct Zero<MultiIndex<d> > {
    typedef MultiIndex<d> Field;
    operator Field() { // does not acctualy exist
      assert(0);
      return Field();
    }
  };
  template <int d>
  bool operator<(const Zero< MultiIndex<d> >& ,const MultiIndex<d>& f) {
    return true;
  }
  template <int d>
  bool operator<(const MultiIndex<d>& f, const Zero< MultiIndex<d> >&) {
    return true;
  }

  // Internal Forward Declarations
  // -----------------------------

  template< class Topology, class F >
  class MonomialBasis;



  // MonomialBasisImpl
  // -----------------

  template< class Topology, class F >
  class MonomialBasisImpl;

  template< class F >
  class MonomialBasisImpl< GenericGeometry::Point, F >
  {
    typedef MonomialBasisImpl< GenericGeometry::Point, F > This;

  public:
    typedef GenericGeometry::Point Topology;
    typedef F Field;

    static const unsigned int dimDomain = Topology::dimension;
    static const unsigned int dimRange = 1;

    typedef FieldVector< Field, dimDomain > DomainVector;
    typedef FieldVector< Field, dimRange > RangeVector;

  private:
    friend class MonomialBasis< Topology, Field >;
    friend class MonomialBasisImpl< GenericGeometry::Prism< Topology >, Field >;
    friend class MonomialBasisImpl< GenericGeometry::Pyramid< Topology >, Field >;

    mutable unsigned int maxOrder_;
    // sizes_[ k ]: number of basis functions of exactly order k
    mutable unsigned int *sizes_;
    // numBaseFunctions_[ k ] = sizes_[ 0 ] + ... + sizes_[ k ]
    mutable unsigned int *numBaseFunctions_;

    MonomialBasisImpl ()
      : sizes_( 0 ),
        numBaseFunctions_( 0 )
    {
      computeSizes( 2 );
    }

    ~MonomialBasisImpl ()
    {
      delete[] sizes_;
      delete[] numBaseFunctions_;
    }

    template< int dimD >
    void evaluate ( const unsigned int order,
                    const FieldVector< Field, dimD > &x,
                    const unsigned int *const offsets,
                    RangeVector *const values ) const
    {
      values[ 0 ] = Unity<Field>();
    }

    void integral ( const unsigned int order,
                    const unsigned int *const offsets,
                    RangeVector *const values ) const
    {
      values[ 0 ] = Field( 1 );
    }

    unsigned int maxOrder () const
    {
      return maxOrder_;
    }

    void computeSizes ( unsigned int order ) const
    {
      maxOrder_ = order;

      delete [] sizes_;
      delete [] numBaseFunctions_;
      sizes_            = new unsigned int [ order+1 ];
      numBaseFunctions_ = new unsigned int [ order+1 ];

      sizes_[ 0 ] = 1;
      numBaseFunctions_[ 0 ] = 1;
      for( unsigned int k = 1; k <= order; ++k )
      {
        sizes_[ k ]            = 0;
        numBaseFunctions_[ k ] = 1;
      }
    }
  };

  template< class BaseTopology, class F >
  class MonomialBasisImpl< GenericGeometry::Prism< BaseTopology >, F >
  {
    typedef MonomialBasisImpl< GenericGeometry::Prism< BaseTopology >, F > This;

  public:
    typedef GenericGeometry::Prism< BaseTopology > Topology;
    typedef F Field;

    static const unsigned int dimDomain = Topology::dimension;
    static const unsigned int dimRange = 1;

    typedef FieldVector< Field, dimDomain > DomainVector;
    typedef FieldVector< Field, dimRange > RangeVector;

  private:
    friend class MonomialBasis< Topology, Field >;
    friend class MonomialBasisImpl< GenericGeometry::Prism< Topology >, Field >;
    friend class MonomialBasisImpl< GenericGeometry::Pyramid< Topology >, Field >;

    MonomialBasisImpl< BaseTopology, Field > baseBasis_;
    // sizes_[ k ]: number of basis functions of exactly order k
    mutable unsigned int *sizes_;
    // numBaseFunctions_[ k ] = sizes_[ 0 ] + ... + sizes_[ k ]
    mutable unsigned int *numBaseFunctions_;

    MonomialBasisImpl ()
      : sizes_( 0 ),
        numBaseFunctions_( 0 )
    {
      computeSizes( 2 );
    }

    ~MonomialBasisImpl ()
    {
      delete[] sizes_;
      delete[] numBaseFunctions_;
    }

    template< int dimD >
    void evaluate ( const unsigned int order,
                    const FieldVector< Field, dimD > &x,
                    const unsigned int *const offsets,
                    RangeVector *const values ) const
    {
      const Field &z = x[ dimDomain-1 ];

      // fill first column
      baseBasis_.evaluate( order, x, offsets, values );
      const unsigned int *const baseSizes = baseBasis_.sizes_;

      RangeVector *row0 = values;
      for( unsigned int k = 1; k <= order; ++k )
      {
        RangeVector *const row1begin = values + offsets[ k-1 ];
        RangeVector *const colkEnd = row1begin + (k+1)*baseSizes[ k ];
        assert( (unsigned int)(colkEnd - values) <= offsets[ k ] );
        RangeVector *const row1End = row1begin + sizes_[ k ];
        assert( (unsigned int)(row1End - values) <= offsets[ k ] );

        RangeVector *row1 = row1begin;
        RangeVector *it;
        for( it = row1begin + baseSizes[ k ]; it != colkEnd; ++row1, ++it )
          *it = z * (*row1);
        for( ; it != row1End; ++row0, ++it )
          *it = z * (*row0);
        row0 = row1;
      }
    }

    void integral ( const unsigned int order,
                    const unsigned int *const offsets,
                    RangeVector *const values ) const
    {
      // fill first column
      baseBasis_.integral( order, offsets, values );
      const unsigned int *const baseSizes = baseBasis_.sizes_;

      RangeVector *row0 = values;
      for( unsigned int k = 1; k <= order; ++k )
      {
        RangeVector *const row1begin = values + offsets[ k-1 ];
        RangeVector *const row1End = row1begin + sizes_[ k ];
        assert( (unsigned int)(row1End - values) <= offsets[ k ] );

        RangeVector *row1 = row1begin;
        RangeVector *it = row1begin + baseSizes[ k ];
        for( unsigned int j = 1; j <= order; ++j )
        {
          RangeVector *const end = it + baseSizes[ k ];
          assert( (unsigned int)(end - values) <= offsets[ k ] );
          for( ; it != end; ++row1, ++it )
            *it = (Field( j ) / Field( j+1 )) * (*row1);
        }
        for( ; it != row1End; ++row0, ++it )
          *it = (Field( k ) / Field( k+1 )) * (*row0);
        row0 = row1;
      }
    }

    unsigned int maxOrder() const
    {
      return baseBasis_.maxOrder();
    }

    void computeSizes ( unsigned int order ) const
    {
      delete[] sizes_;
      delete[] numBaseFunctions_;
      sizes_            = new unsigned int[ order+1 ];
      numBaseFunctions_ = new unsigned int[ order+1 ];

      baseBasis_.computeSizes( order );
      const unsigned int *const baseSizes = baseBasis_.sizes_;
      const unsigned int *const baseNBF   = baseBasis_.numBaseFunctions_;

      sizes_[ 0 ] = 1;
      numBaseFunctions_[ 0 ] = 1;
      for( unsigned int k = 1; k <= order; ++k )
      {
        sizes_[ k ]            = baseNBF[ k ] + k*baseSizes[ k ];
        numBaseFunctions_[ k ] = numBaseFunctions_[ k-1 ] + sizes_[ k ];
      }
    }
  };

  template< class BaseTopology, class F >
  class MonomialBasisImpl< GenericGeometry::Pyramid< BaseTopology >, F >
  {
    typedef MonomialBasisImpl< GenericGeometry::Pyramid< BaseTopology >, F > This;

  public:
    typedef GenericGeometry::Pyramid< BaseTopology > Topology;
    typedef F Field;

    static const unsigned int dimDomain = Topology::dimension;
    static const unsigned int dimRange = 1;

    typedef FieldVector< Field, dimDomain > DomainVector;
    typedef FieldVector< Field, dimRange > RangeVector;

  private:
    friend class MonomialBasis< Topology, Field >;
    friend class MonomialBasisImpl< GenericGeometry::Prism< Topology >, Field >;
    friend class MonomialBasisImpl< GenericGeometry::Pyramid< Topology >, Field >;

    MonomialBasisImpl< BaseTopology, Field > baseBasis_;
    // sizes_[ k ]: number of basis functions of exactly order k
    mutable unsigned int *sizes_;
    // numBaseFunctions_[ k ] = sizes_[ 0 ] + ... + sizes_[ k ]
    mutable unsigned int *numBaseFunctions_;

    MonomialBasisImpl ()
      : sizes_( 0 ),
        numBaseFunctions_( 0 )
    {
      computeSizes( 2 );
    }

    ~MonomialBasisImpl ()
    {
      delete[] sizes_;
      delete[] numBaseFunctions_;
    }

    template< int dimD >
    void evaluateSimplex ( const unsigned int order,
                           const FieldVector< Field, dimD > &x,
                           const unsigned int *const offsets,
                           RangeVector *const values ) const
    {
      const Field &z = x[ dimDomain-1 ];

      // fill first column
      baseBasis_.evaluate( order, x, offsets, values );

      const unsigned int *const baseSizes = baseBasis_.sizes_;
      RangeVector *row0 = values;
      for( unsigned int k = 1; k <= order; ++k )
      {
        RangeVector *const row1 = values+offsets[ k-1 ];
        RangeVector *const row1End = row1+sizes_[ k ];
        assert( (unsigned int)(row1End - values) <= offsets[ k ] );
        for( RangeVector *it = row1 + baseSizes[ k ]; it != row1End; ++row0, ++it )
          *it = z * (*row0);
        row0 = row1;
      }
    }

    template< int dimD >
    void evaluatePyramid ( const unsigned int order,
                           const FieldVector< Field, dimD > &x,
                           const unsigned int *const offsets,
                           RangeVector *const values ) const
    {
      const Field &z = x[ dimDomain-1 ];
      Field omz = Unity<Field>() - z;

      if( Zero<Field>() < omz )
      {
        const Field invomz = Unity<Field>() / omz;
        FieldVector< Field, dimDomain-1 > y;
        for( unsigned int i = 0; i < dimDomain-1; ++i )
          y[ i ] = x[ i ] * invomz;

        // fill first column
        baseBasis_.evaluate( order, y, offsets, values );
      }
      else
        omz = Zero<Field>();

      const unsigned int *const baseSizes = baseBasis_.sizes_;
      RangeVector *row0 = values;
      Field omzk = omz;
      for( unsigned int k = 1; k <= order; ++k )
      {
        RangeVector *const row1 = values + offsets[ k-1 ];
        RangeVector *const row1End = row1 + sizes_[ k ];
        assert( (unsigned int)(row1End - values) <= offsets[ k ] );
        RangeVector *const col0End = row1 + baseSizes[ k ];
        RangeVector *it = row1;
        for( ; it != col0End; ++it )
          *it = (*it) * omzk;
        for( ; it != row1End; ++row0, ++it )
          *it = z * (*row0);
        row0 = row1;
        omzk *= omz;
      }
    }

    template< int dimD >
    void evaluate ( const unsigned int order,
                    const FieldVector< Field, dimD > &x,
                    const unsigned int *const offsets,
                    RangeVector *const values ) const
    {
      if( GenericGeometry::IsSimplex< Topology >::value )
        evaluateSimplex( order, x, offsets, values );
      else
        evaluatePyramid( order, x, offsets, values );
    }

    void integral ( const unsigned int order,
                    const unsigned int *const offsets,
                    RangeVector *const values ) const
    {
      // fill first column
      baseBasis_.integral( order, offsets, values );

      const unsigned int *const baseSizes = baseBasis_.sizes_;
      RangeVector *row0 = values;
      for( unsigned int k = 0; k <= order; ++k )
      {
        const Field factor = (Field( 1 ) / Field( k + dimDomain ));

        RangeVector *const row1 = values+offsets[ k-1 ];
        RangeVector *const col0End = row1 + baseSizes[ k ];
        RangeVector *it = row1;
        for( ; it != col0End; ++it )
          *it = factor * (*it);
        for( unsigned int i = 0; i < k; ++i )
        {
          RangeVector *const end = it + baseSizes[ i ];
          assert( (unsigned int)(end - values) <= offsets[ k ] );
          for( ; it != end; ++row0, ++it )
            *it = (factor * Field( i+1 )) * (*row0);
        }
        row0 = row1;
      }
    }

    unsigned int maxOrder() const
    {
      return baseBasis_.maxOrder();
    }

    void computeSizes ( unsigned int order ) const
    {
      delete[] sizes_;
      delete[] numBaseFunctions_;
      sizes_            = new unsigned int[ order+1 ];
      numBaseFunctions_ = new unsigned int[ order+1 ];

      baseBasis_.computeSizes( order );

      const unsigned int *const baseNBF = baseBasis_.numBaseFunctions_;
      sizes_[ 0 ] = 1;
      numBaseFunctions_[ 0 ] = 1;
      for( unsigned int k = 1; k <= order; ++k )
      {
        sizes_[ k ]            = baseNBF[ k ];
        numBaseFunctions_[ k ] = numBaseFunctions_[ k-1 ] + sizes_[ k ];
      }
    }
  };



  // MonomialBasis
  // -------------

  template< class Topology, class F >
  class MonomialBasis
    : public MonomialBasisImpl< Topology, F >
  {
    typedef MonomialBasis< Topology, F > This;
    typedef MonomialBasisImpl< Topology, F > Base;

  public:
    typedef typename Base::Field Field;

    typedef typename Base::DomainVector DomainVector;
    typedef typename Base::RangeVector RangeVector;

    MonomialBasis ()
      : Base()
    {}

    const unsigned int *sizes ( unsigned int order ) const
    {
      if( order > Base::maxOrder() )
        Base::computeSizes( order );
      return Base::numBaseFunctions_;
    }

    void evaluate ( const unsigned int order,
                    const DomainVector &x,
                    RangeVector *const values ) const
    {
      Base::evaluate( order, x, sizes( order ), values );
    }

    void evaluate ( const unsigned int order,
                    const DomainVector &x,
                    std::vector< RangeVector > &values ) const
    {
      evaluate( order, x, &(values[ 0 ]) );
    }
  };
  template <int dim>
  struct StdMonomialTopology {
    typedef StdMonomialTopology<dim-1> BaseType;
    typedef GenericGeometry::Pyramid<typename BaseType::Type> Type;
  };
  template <>
  struct StdMonomialTopology<0> {
    typedef GenericGeometry::Point Type;
  };
  template <int dim>
  struct StdBiMonomialTopology {
    typedef GenericGeometry::Prism<typename StdBiMonomialTopology<dim-1>::Type> Type;
  };
  template <>
  struct StdBiMonomialTopology<0> {
    typedef GenericGeometry::Point Type;
  };
  template< int dim,class F >
  class StandardMonomialBasis
    : public MonomialBasis< typename StdMonomialTopology<dim>::Type, F >
  {
  public:
    typedef typename StdMonomialTopology<dim>::Type Topology;
  private:
    typedef StandardMonomialBasis< dim, F > This;
    typedef MonomialBasis< Topology, F > Base;
  public:
    StandardMonomialBasis ()
      : Base()
    {}
  };
  template< int dim, class F >
  class StandardBiMonomialBasis
    : public MonomialBasis< typename StdBiMonomialTopology<dim>::Type, F >
  {
  public:
    typedef typename StdBiMonomialTopology<dim>::Type Topology;
  private:
    typedef StandardBiMonomialBasis< dim, F > This;
    typedef MonomialBasis< Topology, F > Base;
  public:
    StandardBiMonomialBasis ()
      : Base()
    {}
  };

}

#endif
