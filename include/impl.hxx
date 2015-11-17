//
// This file is part of The Pivot framework.
// Written by Gabriel Dos Reis <gdr@cs.tamu.edu>
// 

#ifndef IPR_IMPL_INCLUDED
#define IPR_IMPL_INCLUDED

#include <ipr/interface>
#include <ipr/utility>
#include <memory>
#include <list>
#include <vector>
#include <deque>
#include <map>

// -----------------
// -- Methodology --
// -----------------
// This file provides implementations for the interface classes
// defined in <ipr/interface>.  The basic rule is that every class
// found in that header file has at least one implementation in
// this file.  That rule applies to all abstract interfaces such as
// "ipr::Expr", as well as more concrete ones like "ipr::Address".
//    Implementations for the abstract interfaces usually consists in
// prodiving implementations for common operations on some set of
// interfaces.  For instance, "impl::Expr provides implementation for
// the ipr::Expr::type() operation.
//    For that to work properly, without using virtual inheritance,
// we parameterize the implementations of the abstract interfaces with
// (set of) concrete interfaces.  That ensures, for example, that
// impl::Unary overrides ipr::Node::accept() and forward to the right
// ipr::Visitor::visit() hook.


namespace ipr {
   namespace impl {
      // --------------------------------------
      // -- Implementations of ipr::Sequence --
      // --------------------------------------
                                // -- ipr::Sequence --
      // The Sequence<> interface admits various implementations.
      // Here is the list of the implemenations currently in use:
      //   (a) ref_sequence<>; (b) val_sequence<>; 
      //   (c) empty_sequence<T>. 
      // Variants exist in form of
      //   (i) decl_sequence; (ii) singleton_declset<T>.
      

                                // -- impl::ref_sequence --
      // The class ref_sequence<T> implements Sequence<T> by storing
      // references (e.g. pointers) to data of type T, allocated
      // somewhere else.  That for example if useful when keeping track
      // of redelcarations in decl-sets.
      // In general, it can be used to implement the notion of sub-sequence.

      template<class T, class Seq = Sequence<T> >
      struct ref_sequence : Seq, private std::deque<const void*> {
         typedef std::deque<const void*> Rep;
         typedef const T* pointer;
         typedef typename Seq::Iterator Iterator;
         
         explicit ref_sequence(std::size_t n = 0) : Rep(n) { }
         
         // Override ipr::Sequence<T>::size.
         int size() const { return (int)Rep::size(); }
         
         using Seq::operator[];
         using Seq::begin;
         using Seq::end;

         //using Rep::reserve; // YS: deque doesn't support it
         using Rep::resize;
         using Rep::push_back;
         using Rep::push_front;
         
         // Override Cat::get, with range-check.
         const T& get(int p) const { return *pointer(this->at(p)); }
      };

                                // -- impl::val_sequence --
      // The class val_sequence<T> implements Sequence<T> by storing
      // the actual values, instead of references to values (as is
      // the case of ref_sequence<T>).  

      template<class T, class Seq = Sequence<T> >
      struct val_sequence : Seq, private util::slist<T> {
         typedef util::slist<T> Impl;
         typedef typename Seq::Iterator Iterator;

         using Seq::operator[];
         using Seq::begin;
         using Seq::end;

         using Impl::push_back;
         
         int size() const { return Impl::size(); }
         
         const T& get(int p) const
         {
            if (p < 0 || p >= Impl::size())
               throw std::domain_error("val_sequence::get");

            typename Impl::const_iterator b = Impl::begin();
            std::advance(b, p);
            return *b;
         }
      };
                                // -- impl::empty_sequence --
      // There are various situations where the general notion of
      // sequence leads to consider empty sequence in implmentations.
      // We could just use the generic ref_sequence<> or val_sequence<>;
      // however, this specialization will help control data size
      // inflation, as the general sequences need at least two words
      // to keep track of their elements (even when they are empty).
      template<class T>
      struct empty_sequence : ipr::Sequence<T> {
         // Override Sequence<T>::size.
         int size() const { return 0; }
         
         // Override Sequence<T>::get.
         const T& get(int) const
         {
            throw std::domain_error("empty_sequence::get");
         }
      };


      template<class T>
      struct node_ref {
         const T& node;
         explicit node_ref(const T& t) : node(t) { }
      };

                                // -- Node --
      template<class T>
      struct Node : T {
         typedef T Interface;
         void accept(ipr::Visitor& v) const { v.visit(*this); }
      };

      // In various situations, we need to store nodes in ordered
      // containers.  We could of course use the addresses of Nodes as
      // keys.  However, for simplicity and persistency reasons, we chose
      // to used an integer identifier. (Each node  acquires such an
      // identifier at its creation and retain it unchanged till the
      // end of the program.)  An obvious benefit is that ordering comes
      // for free.
      inline int compare(int lhs, int rhs)
      {
         return lhs < rhs ? -1 : (lhs > rhs ? 1 : 0);
      }

      inline int compare(const ipr::Node& lhs, const ipr::Node& rhs)
      {
         return compare(lhs.node_id, rhs.node_id);
      }

      // Helper class
      template<class Interface>
      struct Unary : Interface {
         typedef typename Interface::Arg_type Arg_type;
         Arg_type rep;

         explicit Unary(Arg_type a) : rep(a) { }

         // Override ipr::Unary<>::operand.
         Arg_type operand() const { return rep; }
      };

      template<class Interface>
      struct type_from_operand : impl::Unary<Interface> {
         typedef typename Interface::Arg_type Arg_type;

         explicit type_from_operand(Arg_type a) : impl::Unary<Interface>(a) { }
         const ipr::Type& type() const // override ipr::Expr::type.
         {
            return this->rep.type();
         }
      };
      
      template<class Interface>
      struct Binary : Interface {
         typedef typename Interface::Arg1_type Arg1_type;
         typedef typename Interface::Arg2_type Arg2_type;
         struct Rep {
            Arg1_type first;
            Arg2_type second;
            Rep(Arg1_type f, Arg2_type s) : first(f), second(s) { }
         };
         Rep rep;

         Binary(const Rep& r) : rep(r) { }
         Binary(Arg1_type f, Arg2_type s) : rep(f, s) { }

         // Override ipr::Binary<>::first.
         Arg1_type first() const { return rep.first; }

         // Override ipr::Binary<>::second.
         Arg2_type second() const { return rep.second; }
      };

      template<class Interface>
      struct type_from_second : impl::Binary<Interface> {
         typedef typename Interface::Arg1_type Arg1_type;
         typedef typename Interface::Arg2_type Arg2_type;

         type_from_second(Arg1_type f, Arg2_type s)
               : impl::Binary<Interface>(f, s) { }

         const ipr::Type& type() const // override ipr::Expr::type
         {
            return this->rep.second.type();
         }
      };


      template<class Interface>
      struct Ternary : Interface {
         typedef typename Interface::Arg1_type Arg1_type;
         typedef typename Interface::Arg2_type Arg2_type;
         typedef typename Interface::Arg3_type Arg3_type;
         struct Rep {
            Arg1_type first;
            Arg2_type second;
            Arg3_type third;
            Rep(Arg1_type f, Arg2_type s, Arg3_type t)
                  : first(f), second(s), third(t)
            { }
         };
         Rep rep;

         Ternary(const Rep& r) : rep(r) { }
         Ternary(Arg1_type f, Arg2_type s, Arg3_type t) : rep(f, s, t) { }

         // Override ipr::Ternary<>::first.
         Arg1_type first() const { return rep.first; }

         // Override ipr::Ternary<>::second.
         Arg2_type second() const { return rep.second; }

         // Override ipr::Ternary<>::third.
         Arg3_type third() const { return rep.third; }
      };

      template<class Interface>
      struct Quaternary : Interface {
         typedef typename Interface::Arg1_type Arg1_type;
         typedef typename Interface::Arg2_type Arg2_type;
         typedef typename Interface::Arg3_type Arg3_type;
         typedef typename Interface::Arg4_type Arg4_type;
         struct Rep {
            Arg1_type first;
            Arg2_type second;
            Arg3_type third;
            Arg4_type fourth;
            Rep(Arg1_type f, Arg2_type s, Arg3_type t, Arg4_type l)
                  : first(f), second(s), third(t), fourth(l)
            { }
         };
         Rep rep;

         Quaternary(const Rep& r) : rep(r) { }
         Quaternary(Arg1_type f, Arg2_type s, Arg3_type t) : rep(f, s, t) { }

         // Override ipr::Quaternary<>::first.
         Arg1_type first() const { return rep.first; }

         // Override ipr::Quaternary<>::second.
         Arg2_type second() const { return rep.second; }

         // Override ipr::Quaternary<>::third.
         Arg3_type third() const { return rep.third; }

         // Override ipr::Quaternary<>::fourth.
         Arg4_type fourth() const { return rep.fourth; }
      };

      
      // -------------------------
      // -- Classic expressions --
      // -------------------------

      template<class Operation>
      struct Classic : Operation {
         const ipr::Decl* op_impl;
         Classic() : op_impl(0) { }

         bool has_impl_decl() const
         {
            return op_impl != 0;
         }

         const ipr::Decl& impl_decl() const
         {
            return *util::check(op_impl);
         }
      };

                                // -- Conversion_expr --
      template<class Interface>
      struct Conversion_expr
         : impl::Binary<impl::Classic<impl::Node<Interface> > > {
         typedef impl::Binary<impl::Classic<impl::Node<Interface> > > Base;
         typedef typename Base::Arg1_type Arg1_type;
         typedef typename Base::Arg2_type Arg2_type;

         explicit Conversion_expr(typename Base::Rep r) : Base(r) { }
         Conversion_expr(Arg1_type f, Arg2_type s) : Base(f, s) { }
         const ipr::Type& type() const { return this->rep.first; }
      };


                                // -- String --
      struct String : impl::Node<ipr::String> {
         explicit String(const util::string&);

         int size() const;
         const char* begin() const;
         const char* end() const;
         
      private:
         const util::string& text;
      };

                                // -- Linkage --
      typedef Unary<Node<ipr::Linkage> > Linkage;

      // -------------------------
      // -- General expressions --
      // -------------------------
      // Various generalized expressions are the result of a unary
      // operator applied to an expression.  Their structure is captured
      // by ipr::Unary<>.  The class unary_expr<T> implements that
      // interface.

      template<class Interface>
      struct Expr : impl::Node<Interface> {
         const ipr::Type* constraint;

         Expr(const ipr::Type* t = 0) : constraint(t) { }

         // Override ipr::Expr::type.
         const ipr::Type& type() const { return *util::check(constraint); }
      };


      // ----------------------------------------------------
      // -- Scopes, sequence of declarations and overloads --
      // ----------------------------------------------------
      //
      // A scope is defined to be a sequence of declarations.
      // There may be multiple instances of a declaration in
      // a scope.  All declarations sharing the same name
      // form an overload set.  Consequently a scope is partitioned
      // into overload sets, each of which is uniquely identified
      // by its name.
      // An overload set, in turn, is partitioned in sub-sets of
      // (multiple) declarations with same type.  So, each decl-set
      // within its overload set, is uniquely determined by its
      // type.  Each decl-set has a "standard" representative,
      // called the "master declaration".  A master declaration is
      // the first seen declaration of a decl-set.
      //
      // There are few special cases of the above general description.
      // Parameters, base-classes and enumerators cannot be mutiply
      // declared.  Therefore within a parameter-lits, a base-class
      // list or an enumeration definition, each decl-set is
      // singleton whose sole element is the master declaration.
      //
      // A scope chains declaration together.  A declaration in a
      // Scope has a "position", that uniquely identifies it as a member
      // of a sequence.  To provide a relatively fast "subcription by position"
      // operation on a scope, the chain of declaration is
      // organized as a red-back tree where the position is used as key.
      // And alternative is a hash table, when we get there.
      
      using util::rb_tree::link;
      struct scope_datum : link<scope_datum> {
         // The position of this Decl in its scope.  It shall be set
         // at the actual declaration creation by the creating scope.
         int scope_pos;

         // The specifiers for this declaration.  S
         ipr::Decl::Specifier spec;

         // Back-pointer to this declaration.  It shall be set at
         // the declaration creation.
         const ipr::Decl* decl; 

         scope_datum() : scope_pos(-1), spec(ipr::Decl::None), decl(0)
         { }

         // A comparator object type, implements ordering on declaration
         // position chaining declarations together, in scopes.
         struct comp;
      };

                                // -- impl::decl_sequence --
      // The chain of declarations in a scope.
      
      struct decl_sequence : ipr::Sequence<ipr::Decl> {
         // Override ipr::Sequence<>::size.
         int size() const;

         // Override ipr::Sequence<>::get.
         const ipr::Decl& get(int) const;

         // Inserts a declaration in this sequence.
         void insert(scope_datum*);

      private:
         util::rb_tree::chain<scope_datum> decls;
      };

                                // -- impl::singleton_declset --
      // A singleton_declset is a specialization of decl_sequence
      // that contains only a single declaration.  It is mostly used
      // to support the general interface of ipr::Scope as inherited
      // in parameter-list or enmerator definitions.

      template<class T>
      struct singleton_declset : ipr::Sequence<ipr::Decl> {
         const T& datum;

         explicit singleton_declset(const T& t) : datum(t) { }

         // Override ipr::Sequence<>::size.
         int size() const { return 1; }

         // Override ipr::Sequence::get.
         const T& get(int i) const
         {
            if (i == 1)
               return datum;
            throw std::domain_error("singleton_declset::get");
         }
      };

      
      // All declarations (except parameters, base-classes, enumerations)
      // maintain data about their position and master declaration.  Since
      // all declarations in a given decl-set have the same type, only
      // the master declaration has the "type" information.
      // Similarly, only the master declaration maintains the home
      // region information.
      // 
      // In a given decl-set, only one of the declarations is a definition.
      // The master declaration keeps track of that definition, so all
      // other redeclarations can refer to it through the master
      // declaration data.
      
      template<class> struct master_decl_data;
      struct Overload;

      // An entry in an overload set.  Part of the data that a
      // master declataion manages.  It is determined, within
      // an overload set, by its type.  All redeclarations must
      // register themselves before the master declaration, at
      // the creation time.

      struct overload_entry : link<overload_entry> {
         const ipr::Type& type;
         ref_sequence<ipr::Decl> declset;
         explicit overload_entry(const ipr::Type& t) : type(t) { }
      };


      template<class Interface>
      struct basic_decl_data : scope_datum {
         // Information about the master declararion.  This pointer
         // shall be set at actual declaration creation time.
         master_decl_data<Interface>* master_data;

         explicit basic_decl_data(master_decl_data<Interface>* mdd)
               : master_data(mdd)
         { }
      };


      // A master declaration is the first seen introduction of
      // a name with a given type.  Further redeclarations
      // are represented by basic_decl_data<> and are linked to the
      // master declaration.  That forms the declaration-set of
      // that declaration.

      template<class Interface>
      struct master_decl_data : basic_decl_data<Interface>, overload_entry {
         // The declaration that is considered to be the definition.
         const Interface* def;
         const ipr::Linkage* langlinkage;

         // The overload set that contains this master declaration.  It
         // shall be set at the time the node for the master declaration
         // is created.
         impl::Overload* overload;
         const ipr::Region* home;
         master_decl_data(impl::Overload* ovl, const ipr::Type& t)
               : basic_decl_data<Interface>(this),
                 overload_entry(t),
                 def(0), langlinkage(0),
                 overload(ovl),home(0)
         { } 
      };

      template<>
      struct master_decl_data<ipr::Named_map>
         : basic_decl_data<ipr::Named_map>, overload_entry {
         typedef basic_decl_data<ipr::Named_map> Base;
         // The declaration that is considered to be the definition.
         const ipr::Named_map* def;
         const ipr::Linkage* langlinkage;
         const ipr::Named_map* primary;
         const ipr::Region* home;

         // The overload set that contains this master declaration.  It
         // shall be set at the time the node for the master declaration
         // is created.
         impl::Overload* overload;

         // Sequence of specilizations
         decl_sequence specs;

         master_decl_data(impl::Overload*, const ipr::Type&);
      };
      
      
      struct Overload : impl::Expr<ipr::Overload> {
         // The name of this overload set.
         const ipr::Name& name;

         // All declarations happen in some region.  This is the region
         // that contains the declarations in this overload set.  It shall
         // be set at the overload set is actually created.
         const ipr::Region* where;

         // All entries in this overload set, chained together in a
         // red-back tree to permit fast retrieval based on type (as key).
         // They are all master declarations.
         util::rb_tree::chain<overload_entry> entries;

         // A sequence of master declarations.  They are added as they
         // appear in their enclosing scope.
         std::vector<scope_datum*> masters;

         explicit Overload(const ipr::Name&);

         // Override ipr::Overload::operator[](const ipr::Type&).
         const ipr::Sequence<ipr::Decl>& operator[](const ipr::Type&) const;

         // Override ipr::Sequence<ipr::Decl>::size.
         int size() const;

         // Override ipr::Sequence<ipr::Decl>::get.
         const ipr::Decl& get(int) const;

         overload_entry* lookup(const ipr::Type&) const;

         template<class T>
         void push_back(master_decl_data<T>*);
      };

      // Parameters, base-subobjects and enumerations cannot be
      // multiply declared in a given region.  Therefore such a
      // declaration is the sole member in its own decl-set.
      // Furthermore, there cannot be other declaration with
      // different type but same name.  Therefore the name for
      // such a declaration defines an overload set with a single
      // member.  This class implements such a special overload set.
      
      struct singleton_overload : impl::Node<ipr::Overload> {
         singleton_declset<ipr::Decl> seq;

         explicit singleton_overload(const ipr::Decl&);

         // Override ipr::Expr::type.
         const ipr::Type& type() const;

         // Override ipr::Sequence<ipr::Decl>::size.
         int size() const;

         // Override ipr::Sequence<ipr::Decl>::get.
         const ipr::Decl& get(int) const;

         // Override ipr::Overload::operator[](const ipr::Type&).
         const ipr::Sequence<ipr::Decl>& operator[](const ipr::Type&) const;
      };


      // When a name is not declared in a scope, then an empty overload
      // set is returned.  This class empty_overload implements that notion.
      // Alternatively, we could have thrown an exception to indicate
      // that failure; however, the resulting programming style might
      // be clutered by try-blocks. 

      struct empty_overload : impl::Node<ipr::Overload> {
         // Override ipr::Expr::type.
         const ipr::Type& type() const;

         // Override ipr::Sequence<ipr::Decl>::size.
         int size() const;

         // Override ipr::Sequence<ipr::Decl>::get.
         const ipr::Decl& get(int) const;

         // Override ipr::Overload::operator[](const ipr::Type&).
         const ipr::Sequence<ipr::Decl>& operator[](const ipr::Type&) const;
      };

      struct node_compare {
         int operator()(const ipr::Node& lhs, const ipr::Node& rhs) const
         {
            return compare(lhs, rhs);
         }

         int
         operator()(const ipr::Type& t, const overload_entry& e) const
         {
            return (*this)(t, e.type);
         }

         int
         operator()(const overload_entry& l, const overload_entry& r) const
         {
            return (*this)(l.type, r.type);
         }
      };

      // -----------------------
      // -- Generalized types --
      // -----------------------

      // impl::Type<T> implements the common operations supported
      // by all operations.  In particular, it implements only
      // main-variant types; qualified types are handled elsewere.

      template<class T>
      struct Type : impl::Expr<T> {
         Type() : id(0) { }
         
         // override ipr::Type::qualifiers.
//         ipr::Type::Qualifier qualifiers() const { return ipr::Type::None; }

         // override ipr::Type::main_variant.
//         const ipr::Type& main_variant() const { return *this; }

         // override ipr::Type::name.
         const ipr::Name& name() const { return *util::check(id); }

         const ipr::Name* id;
      };

      // Scopes, as expressions, have Product types.  Such types could
      // be implemented directly as a separate sequence of types.
      // However, it would require coordination from the various
      // scope updaters to keep the types in sync.  An alternative,
      // implemented by typed_sequence<Seq>, is to wrap sequences
      // in "type envelop" and return the nth type as being the
      // type of the nth declaration in the sequence.

      template<class Seq>
      struct typed_sequence : impl::Type<ipr::Product>,
                              ipr::Sequence<ipr::Type> {
         Seq seq;

         typed_sequence() { }
         explicit typed_sequence(const Seq& s) : seq(s) { }
         
         // override ipr::Unary<>::operand.
         const ipr::Sequence<ipr::Type>& operand() const { return *this; }

         // override ipr::Sequence<>::size.
         int size() const { return seq.size(); }

         // override ipr::Sequence<>::get.
         const ipr::Type& get(int i) const { return seq.get(i).type(); }
      };

      // This class template maps an interface type to its actual
      // implementation type.  It is mostly used for getting the
      // implementation type of declaration interfaces.
      
      template<class>
      struct traits {
      };

                                // -----------------
                                // -- impl::Rname --
                                // -----------------
      struct Rname : Ternary<Node<ipr::Rname> > {
         explicit Rname(Rep);

         // It was not strictly necessary to actually define this class
         // and the function type().  See the comments in <ipr/interface>.
         const ipr::Type& type() const;
      };
      
      // Stmt<S> implements the common operations of statements.

      template<class S>
      struct Stmt : S {
         ipr::Unit_location unit_locus;
         ipr::Source_location src_locus;
         ref_sequence<ipr::Annotation> notes;

         // Override ipr::Stmt::unit_location.
         const ipr::Unit_location& unit_location() const
         {
            return unit_locus;
         }

         // Override ipr::Stmt::unit_location.
         const ipr::Source_location& source_location() const
         {
            return src_locus;
         }

         // Override ipr::Stmt::annotations.
         const ipr::Sequence<ipr::Annotation>& annotation() const
         {
            return notes;
         }
      };

      // The class template Decl<> implements common operations
      // for most declarations nodes.  The exception cases are
      // handled by the class template unique_decl<>.
      
      template<class D>
      struct Decl : Stmt<Node<D> > {
         basic_decl_data<D> decl_data;
         ipr::Named_map* pat;
         val_sequence<ipr::Substitution> args;

         Decl() : decl_data(0), pat(0) { }

         // Override Decl::substitutions.
         const ipr::Sequence<ipr::Substitution>& substitutions() const
         { return args; }

         // Override Decl::pattern;
         const ipr::Named_map& generating_map() const
         { return *util::check(pat); }

         const ipr::Linkage& lang_linkage() const
         {
            return *util::check(util::check(decl_data.master_data)->langlinkage);
         }

         const ipr::Region& home_region() const {
            return *util::check(util::check(decl_data.master_data)->home);
         }

         // Set declaration specifiers for this decl.
         void specifiers(ipr::Decl::Specifier s) {
            decl_data.spec = s;
         }
      };


      // Some declarations (e.g. parameters, base-classes, enumerators)
      // cannot be redeclared in their declarative regions.  Consequently
      // their decl-sets and overload sets are singleton, containing
      // only the mater declarations.  Consequently, it seems
      // heavyweight space wasteful to deploy the general representation
      // machinery for them.  The class unique_decl<> implements the
      // specialization of Decl<> in those cases.
      
      template<class Interface>
      struct unique_decl : impl::Stmt<Node<Interface> > {
         ipr::Decl::Specifier spec;
         const ipr::Linkage* langlinkage;
         singleton_overload overload;
         ipr::Named_map* pat;
         val_sequence<ipr::Substitution> args;

         unique_decl() : spec(ipr::Decl::None),
                         langlinkage(0),
                         overload(*this),
                         pat(0)
         { }

         ipr::Decl::Specifier specifiers() const { return spec; }
         const ipr::Decl& master() const { return *this; }
         const ipr::Linkage& lang_linkage() const
         {
            return *util::check(langlinkage);
         }

         // Override Decl::substitutions.
         const ipr::Sequence<ipr::Substitution>& substitutions() const
         { return args; }

         // Override Decl::pattern;
         const ipr::Named_map& generating_map() const
         { return *util::check(pat); }
      };

      struct Parameter : unique_decl<ipr::Parameter> {
         const ipr::Name& id;
         const impl::Rname& abstract_name;
         const ipr::Parameter_list* where;
         const ipr::Expr* init;

         Parameter(const ipr::Name&, const impl::Rname&);
         
         const ipr::Name& name() const;
         const ipr::Type& type() const;
         const ipr::Region& home_region() const;
         const ipr::Region& lexical_region() const;
         const ipr::Sequence<ipr::Decl>& decl_set() const;
         int position() const;
         const ipr::Expr& initializer() const;
         bool has_initializer() const;
         // FIXME: This should go away.
         const Parameter_list& membership() const;
      };

      struct Base_type : unique_decl<ipr::Base_type> {
         const ipr::Type& base;
         const ipr::Region& where;
         const int scope_pos;

         Base_type(const ipr::Type&, const ipr::Region&, int);
         const ipr::Type& type() const;
         const ipr::Region& lexical_region() const;
         const ipr::Region& home_region() const;
         int position() const;
         const ipr::Expr& initializer() const;
         bool has_initializer() const;
         const ipr::Sequence<ipr::Decl>& decl_set() const;
      };

      struct Enumerator : unique_decl<ipr::Enumerator> {
         const ipr::Name& id;
         const ipr::Enum& constraint;
         const int scope_pos;
         const ipr::Region* where;
         const ipr::Expr* init;

         Enumerator(const ipr::Name&, const ipr::Enum&, int);
         const ipr::Name& name() const;
         const ipr::Region& lexical_region() const;
         const ipr::Region& home_region() const;
         const ipr::Sequence<ipr::Decl>& decl_set() const;
         int position() const;
         const ipr::Expr& initializer() const;
         bool has_initializer() const;
         // FIXME: this should go away.
         const ipr::Enum& membership() const;
      };

      
      // Map ipr::Parameter, ipr::Base_type and ipr::Enumerator
      // to their implementation classes.  This information is
      // mostly used in the implementation of homogeneous_sequence,
      // homogeneous_scope and homogeneous_region.

      template<>
      struct traits<ipr::Parameter> {
         typedef impl::Parameter rep;
      };

      template<>
      struct traits<ipr::Base_type> {
         typedef impl::Base_type rep;
      };

      template<>
      struct traits<ipr::Enumerator> {
         typedef impl::Enumerator rep;
      };


      // A sequence of homogenous node can be represented directly
      // as a container of the concrete implementations classes
      // instad of pointers to the interface nodes.  This is the
      // case in particular for enumerators or parameters.
      
      template<class Member>
      struct homogeneous_sequence : ipr::Sequence<Member> {
         typedef typename traits<Member>::rep rep;
         val_sequence<rep> seq;

         int size() const { return seq.size(); }
         const rep& get(int i) const { return seq.get(i); }


         // Most nodes in homogeneous sequences need only
         // two or three arguments for their constructors.
         template<class T, class U>
         rep* push_back(const T& t, const U& u)
         {
            return seq.push_back(t, u);
         }

         template<class T, class U, class V>
         rep* push_back(const T& t, const U& u, const V& v)
         {
            return seq.push_back(t, u, v);
         }
      };

      
      template<class Member>
      struct homogeneous_scope : impl::Node<ipr::Scope>,
                                 ipr::Sequence<ipr::Decl>,
                                 std::allocator<void> {
         typedef typename homogeneous_sequence<Member>::rep member_rep;
         typed_sequence<homogeneous_sequence<Member> > decls;
         empty_overload missing;

         explicit
         homogeneous_scope(const ipr::Type& t)
         {
            decls.constraint = &t;
         }

         int size() const { return decls.size(); }
         const Member& get(int i) const { return decls.seq.get(i); }

         const ipr::Product& type() const { return decls; }

         const ipr::Sequence<ipr::Decl>& members() const { return *this; }

         const ipr::Overload& operator[](const Name&) const;

         template<class T, class U>
         member_rep* push_back(const T& t, const U& u)
         {
            member_rep* decl = decls.seq.push_back(t, u);
            return decl;
         }

         template<class T, class U, class V>
         member_rep* push_back(const T& t, const U& u, const V& v)
         {
            member_rep* decl = decls.seq.push_back(t, u, v);
            return decl;
         }
      };

      // FIXME: Remove this linear search.
      template<class Member>
      const ipr::Overload&
      homogeneous_scope<Member>::operator[](const ipr::Name& n) const
      {
         const int s = decls.size();
         for (int i = 0; i < s; ++i) {
            const typename homogeneous_sequence<Member>::rep&
               decl = decls.seq.get(i);
            if (decl.name().node_id == n.node_id)
               return decl.overload;
         }

         return missing;
      }

      template<class Member,
               class RegionKind = Node<ipr::Region> >
      struct homogeneous_region : RegionKind {
         typedef ipr::Region::Location_span location_span;
         const ipr::Region& parent;
         location_span extent;
         const ipr::Expr* owned_by;
         homogeneous_scope<Member> scope;

         int size() const { return scope.size(); }
         const Member& get(int i) const { return scope.get(i); }
         const ipr::Product& type() const { return scope.type(); }

         const ipr::Region& enclosing() const { return parent; }
         const ipr::Scope& bindings() const { return scope; }
         const location_span& span() const { return extent; }
         const ipr::Expr& owner() const { return *util::check(owned_by); }

         homogeneous_region(const ipr::Region& p, const ipr::Type& t)
               : parent(p), owned_by(0), scope(t)
         { }
      };
      
      struct Parameter_list : homogeneous_region<ipr::Parameter,
                                 impl::Node<ipr::Parameter_list> > {
         typedef homogeneous_region<ipr::Parameter,
                                    impl::Node<ipr::Parameter_list> > Base;
         
         Parameter_list(const ipr::Region&, const ipr::Type&);

         impl::Parameter* add_member(const ipr::Name&, const impl::Rname&);
      };

      template<class T>
      struct decl_rep : traits<T>::rep {
         decl_rep(master_decl_data<T>* mdd)
         {
            this->decl_data.master_data = mdd;
            this->decl_data.decl = this;
            mdd->declset.push_back(this);
         }
         
         const ipr::Name& name() const
         {
            return this->decl_data.master_data->overload->name;
         }
         
         ipr::Decl::Specifier specifiers() const
         {
            return this->decl_data.spec;
         }
         
         const ipr::Type& type() const
         {
            return this->decl_data.master_data->type;
         }
         
         const ipr::Scope& scope() const
         {
            return this->decl_data.master_data->overload->where;
         }

         int position() const
         {
            return this->decl_data.scope_pos;
         }

         const ipr::Decl& master() const
         {
            return *util::check(this->decl_data.master_data->decl);
         }

         const ipr::Sequence<ipr::Decl>& decl_set() const
         {
            return this->decl_data.master_data->declset;
         }

         const T& definition() const
         {
            return *util::check(this->decl_data.master_data->def);
         }
      };

      
      typedef Binary<Type<ipr::Array> > Array;
      typedef Unary<Type<ipr::Decltype> > Decltype;
      typedef Binary<Type<ipr::As_type> > As_type;
      typedef Quaternary<Type<ipr::Function> > Function;
      typedef Unary<Type<ipr::Pointer> > Pointer;
      typedef Unary<Type<ipr::Product> > Product;
      typedef Binary<Type<ipr::Ptr_to_member> > Ptr_to_member;
      typedef Binary<Type<ipr::Qualified> > Qualified;
      typedef Unary<Type<ipr::Reference> > Reference;
      typedef Unary<Type<ipr::Rvalue_reference> > Rvalue_reference;
      typedef Unary<Type<ipr::Sum> > Sum;
      typedef Binary<Type<ipr::Template> > Template;

      typedef Unary<Node<Comment> > Comment;

      typedef Expr<ipr::Phantom> Phantom;

      typedef Unary<Classic<Expr<ipr::Address> > > Address;
      typedef Unary<Classic<Expr<ipr::Array_delete> > > Array_delete;
      typedef Unary<Classic<Expr<ipr::Complement> > > Complement;
      typedef Unary<Expr<ipr::Conversion> > Conversion;
      typedef Unary<Expr<ipr::Ctor_name> > Ctor_name;
      typedef Unary<Classic<Expr<ipr::Delete> > > Delete;
      typedef Unary<Classic<Expr<ipr::Deref> > > Deref;
      typedef Unary<Expr<ipr::Dtor_name> > Dtor_name;

      struct Expr_list : impl::Node<ipr::Expr_list> {
         typed_sequence<ref_sequence<ipr::Expr> > seq;

         Expr_list();
         explicit Expr_list(const ref_sequence<ipr::Expr>&);

         // Override ipr::Expr::type.
         const ipr::Product& type() const;
         
         // Override ipr::Unary<>::operand.
         const ipr::Sequence<ipr::Expr>& operand() const;

         void push_back(const ipr::Expr* e) { seq.seq.push_back(e); }
		 // >>>> Yuriy Solodkyy: 2007/02/02 
		 // Front insertable sequence is more suitable for bottom-up parsing
		 void push_front(const ipr::Expr* e) { seq.seq.push_front(e); }
		 // <<<< Yuriy Solodkyy: 2007/02/02 
      };

      typedef Unary<Classic<Expr<ipr::Initializer_list> > > Initializer_list;
      typedef Unary<Classic<Expr<ipr::Expr_sizeof> > > Expr_sizeof;
      typedef Unary<Classic<Expr<ipr::Expr_typeid> > > Expr_typeid;
      typedef Unary<Classic<Expr<ipr::Identifier> > > Identifier;

      struct Id_expr : impl::Unary<Expr<ipr::Id_expr> > {
         const ipr::Decl* decl;

         explicit Id_expr(const ipr::Name&);
         const ipr::Type& type() const; // override ipr::Expr::Type.
         const ipr::Decl& resolution() const; //  ipr::Id_expr::resolution.
      };

      typedef Unary<Classic<Expr<ipr::Not> > > Not;

      typedef Unary<Classic<Expr<ipr::Operator> > > Operator;
      typedef type_from_operand<Classic<Node<ipr::Paren_expr> > > Paren_expr;
      
      typedef Unary<Classic<Expr<ipr::Pre_decrement> > > Pre_decrement;
      typedef Unary<Classic<Expr<ipr::Pre_increment> > > Pre_increment;
      typedef Unary<Classic<Expr<ipr::Post_decrement> > > Post_decrement;
      typedef Unary<Classic<Expr<ipr::Post_increment> > > Post_increment;
      typedef Unary<Classic<Expr<ipr::Throw> > > Throw;
      typedef type_from_operand<Node<ipr::Type_id> > Type_id;

      typedef Unary<Classic<Expr<ipr::Type_sizeof> > > Type_sizeof;
      typedef Unary<Classic<Expr<ipr::Type_typeid> > > Type_typeid;
      typedef Unary<Classic<Expr<ipr::Unary_minus> > > Unary_minus;
      typedef Unary<Classic<Expr<ipr::Unary_plus> > > Unary_plus;

      typedef Binary<Classic<Expr<ipr::And> > > And;
      typedef Binary<Node<ipr::Annotation> > Annotation;
      typedef Binary<Classic<Expr<ipr::Array_ref> > > Array_ref;
      typedef Binary<Classic<Expr<ipr::Arrow> > > Arrow;
      typedef Binary<Classic<Expr<ipr::Arrow_star> > > Arrow_star;
      typedef Binary<Classic<Expr<ipr::Assign> > > Assign;
      typedef Binary<Classic<Expr<ipr::Bitand> > > Bitand;
      typedef Binary<Classic<Expr<ipr::Bitand_assign> > > Bitand_assign;
      typedef Binary<Classic<Expr<ipr::Bitor> > > Bitor;
      typedef Binary<Classic<Expr<ipr::Bitor_assign> > > Bitor_assign;
      typedef Binary<Classic<Expr<ipr::Bitxor> > > Bitxor;
      typedef Binary<Classic<Expr<ipr::Bitxor_assign> > > Bitxor_assign;
      typedef Binary<Classic<Expr<ipr::Call> > > Call;
      typedef Conversion_expr<ipr::Cast> Cast;
      typedef Binary<Classic<Expr<ipr::Comma> > > Comma;
      typedef Conversion_expr<ipr::Const_cast> Const_cast;
      typedef Conversion_expr<ipr::Datum> Datum;
      typedef Binary<Classic<Expr<ipr::Div> > > Div;
      typedef Binary<Classic<Expr<ipr::Div_assign> > > Div_assign;
      typedef Binary<Classic<Expr<ipr::Dot> > > Dot;
      typedef Binary<Classic<Expr<ipr::Dot_star> > > Dot_star;
      typedef Conversion_expr<ipr::Dynamic_cast> Dynamic_cast;
      typedef Binary<Classic<Expr<ipr::Equal> > > Equal;
      typedef Binary<Classic<Expr<ipr::Greater> > > Greater;
      typedef Binary<Classic<Expr<ipr::Greater_equal> > > Greater_equal;
      typedef Binary<Classic<Expr<ipr::Less> > > Less;
      typedef Binary<Classic<Expr<ipr::Less_equal> > > Less_equal;
      typedef Conversion_expr<ipr::Literal> Literal;
      typedef Binary<Classic<Expr<ipr::Lshift> > > Lshift;
      typedef Binary<Classic<Expr<ipr::Lshift_assign> > > Lshift_assign;

      struct Mapping : impl::Expr<ipr::Mapping> {
         impl::Parameter_list parameters;
         const ipr::Type* value_type;
         const ipr::Expr* body;
         const int nesting_level;

         Mapping(const ipr::Region&, const ipr::Type&, int);
         
         // Override ipr::Mapping::params.
         const ipr::Parameter_list& params() const;

         // Implement ipr::Mapping::result_type.
         const ipr::Type& result_type() const;

         // Override ipr::Mapping::result.
         const ipr::Expr& result() const;

         // Override ipr::Mapping::depth.
         int depth() const;

         impl::Parameter* param(const ipr::Name&, const impl::Rname&);
      };

      typedef Binary<Expr<ipr::Member_init> > Member_init;
      typedef Binary<Classic<Expr<ipr::Minus> > > Minus;
      typedef Binary<Classic<Expr<ipr::Minus_assign> > > Minus_assign;
      typedef Binary<Classic<Expr<ipr::Modulo> > > Modulo;
      typedef Binary<Classic<Expr<ipr::Modulo_assign> > > Modulo_assign;
      typedef Binary<Classic<Expr<ipr::Mul> > > Mul;
      typedef Binary<Classic<Expr<ipr::Mul_assign> > > Mul_assign;
      typedef Binary<Classic<Expr<ipr::Not_equal> > > Not_equal;
      typedef Binary<Classic<Expr<ipr::Or> > > Or;
      typedef Binary<Classic<Expr<ipr::Plus> > > Plus;
      typedef Binary<Classic<Expr<ipr::Plus_assign> > > Plus_assign;
      typedef Conversion_expr<ipr::Reinterpret_cast> Reinterpret_cast;
      typedef Binary<Classic<Expr<ipr::Rshift> > > Rshift;
      typedef Binary<Classic<Expr<ipr::Rshift_assign> > > Rshift_assign;
      typedef Binary<Expr<ipr::Scope_ref> > Scope_ref;
      typedef Conversion_expr<ipr::Static_cast> Static_cast;
      typedef Binary<Expr<ipr::Template_id> > Template_id;

      typedef Ternary<Classic<Expr<ipr::New> > > New;
      typedef Ternary<Classic<Expr<ipr::Conditional> > > Conditional;

      struct expr_factory {
         // Returns an IPR node for unified string literals.
         const ipr::String& get_string(const char*);
         const ipr::String& get_string(const std::string&);

         // Returns an IPR node a language linkage.
         const ipr::Linkage& get_linkage(const char*);
         const ipr::Linkage& get_linkage(const std::string&);
         const ipr::Linkage& get_linkage(const ipr::String&);

         Annotation* make_annotation(const ipr::String&, const ipr::Literal&);

         // Build a node for a missing expression of an unspecified type.
         Phantom* make_phantom();
         // Build an unspecified expression node of a given type.
         const ipr::Phantom* make_phantom(const ipr::Type&);

         // Returns an IPR node for a typed literal expression.
         Literal* make_literal(const ipr::Type&, const ipr::String&);
         Literal* make_literal(const ipr::Type&, const char*);
         Literal* make_literal(const ipr::Type&, const std::string&);

         // Builds an IPR object for an identifier.
         Identifier* make_identifier(const ipr::String&);
         Identifier* make_identifier(const char*);
         Identifier* make_identifier(const std::string&);

         // Builds an IPR object for an operator name.
         Operator* make_operator(const ipr::String&);
         Operator* make_operator(const char*);
         Operator* make_operator(const std::string&);

         Address* make_address(const ipr::Expr&);
         Array_delete* make_array_delete(const ipr::Expr&);
         Complement* make_complement(const ipr::Expr&);
         Conversion* make_conversion(const ipr::Type&);
         Ctor_name* make_ctor_name(const ipr::Type&);
         Delete* make_delete(const ipr::Expr&);
         Deref* make_deref(const ipr::Expr&);
         Dtor_name* make_dtor_name(const ipr::Type&);
         Expr_list* make_expr_list();
         Expr_sizeof* make_expr_sizeof(const ipr::Expr&);
         Expr_typeid* make_expr_typeid(const ipr::Expr&);
         Initializer_list* make_initializer_list(const ipr::Expr_list&);
         impl::Id_expr* make_id_expr(const ipr::Name&);
         Id_expr* make_id_expr(const ipr::Decl&);
         Not* make_not(const ipr::Expr&);
         Paren_expr* make_paren_expr(const ipr::Expr&);
         Post_increment* make_post_increment(const ipr::Expr&);
         Post_decrement* make_post_decrement(const ipr::Expr&);
         Pre_increment* make_pre_increment(const ipr::Expr&);
         Pre_decrement* make_pre_decrement(const ipr::Expr&);
         Throw* make_throw(const ipr::Expr&);
         Type_id* make_type_id(const ipr::Type&);
         Type_sizeof* make_type_sizeof(const ipr::Type&);
         Type_typeid* make_type_typeid(const ipr::Type&);
         Unary_minus* make_unary_minus(const ipr::Expr&);
         Unary_plus* make_unary_plus(const ipr::Expr&);

         And* make_and(const ipr::Expr&, const ipr::Expr&);
         Array_ref* make_array_ref(const ipr::Expr&, const ipr::Expr&);
         Arrow* make_arrow(const ipr::Expr&, const ipr::Expr&);
         Arrow_star* make_arrow_star(const ipr::Expr&, const ipr::Expr&);
         Assign* make_assign(const ipr::Expr&, const ipr::Expr&);
         Bitand* make_bitand(const ipr::Expr&, const ipr::Expr&);
         Bitand_assign* make_bitand_assign(const ipr::Expr&, const ipr::Expr&);
         Bitor* make_bitor(const ipr::Expr&, const ipr::Expr&);
         Bitor_assign* make_bitor_assign(const ipr::Expr&, const ipr::Expr&);
         Bitxor* make_bitxor(const ipr::Expr&, const ipr::Expr&);
         Bitxor_assign* make_bitxor_assign(const ipr::Expr&, const ipr::Expr&);
         Cast* make_cast(const ipr::Type&, const ipr::Expr&);
         Call* make_call(const ipr::Expr&, const ipr::Expr_list&);
         Comma* make_comma(const ipr::Expr&, const ipr::Expr&);
         Const_cast* make_const_cast(const ipr::Type&, const ipr::Expr&);
         Datum* make_datum(const ipr::Type&, const ipr::Expr_list&);
         Div* make_div(const ipr::Expr&, const ipr::Expr&);
         Div_assign* make_div_assign(const ipr::Expr&, const ipr::Expr&);
         Dot* make_dot(const ipr::Expr&, const ipr::Expr&);
         Dot_star* make_dot_star(const ipr::Expr&, const ipr::Expr&);
         Dynamic_cast* make_dynamic_cast(const ipr::Type&, const ipr::Expr&);
         Equal* make_equal(const ipr::Expr&, const ipr::Expr&);
         Greater* make_greater(const ipr::Expr&, const ipr::Expr&);
         Greater_equal* make_greater_equal(const ipr::Expr&, const ipr::Expr&);
         Less* make_less(const ipr::Expr&, const ipr::Expr&);
         Less_equal* make_less_equal(const ipr::Expr&, const ipr::Expr&);
         Lshift* make_lshift(const ipr::Expr&, const ipr::Expr&);
         Lshift_assign* make_lshift_assign(const ipr::Expr&, const ipr::Expr&);
         Member_init* make_member_init(const ipr::Expr&, const ipr::Expr&);
         Minus* make_minus(const ipr::Expr&, const ipr::Expr&);
         Minus_assign* make_minus_assign(const ipr::Expr&, const ipr::Expr&);
         Modulo* make_modulo(const ipr::Expr&, const ipr::Expr&);
         Modulo_assign* make_modulo_assign(const ipr::Expr&, const ipr::Expr&);
         Mul* make_mul(const ipr::Expr&, const ipr::Expr&);
         Mul_assign* make_mul_assign(const ipr::Expr&, const ipr::Expr&);
         Not_equal* make_not_equal(const ipr::Expr&, const ipr::Expr&);
         Or* make_or(const ipr::Expr&, const ipr::Expr&);
         Plus* make_plus(const ipr::Expr&, const ipr::Expr&);
         Plus_assign* make_plus_assign(const ipr::Expr&, const ipr::Expr&);
         Reinterpret_cast* make_reinterpret_cast(const ipr::Type&,
                                                 const ipr::Expr&);
         Scope_ref* make_scope_ref(const ipr::Expr&, const ipr::Expr&);
         Rshift* make_rshift(const ipr::Expr&, const ipr::Expr&);
         Rshift_assign* make_rshift_assign(const ipr::Expr&, const ipr::Expr&);
         Template_id* make_template_id(const ipr::Name&,
                                       const ipr::Expr_list&);
         Static_cast* make_static_cast(const ipr::Type&, const ipr::Expr&);
         New* make_new(const ipr::Expr_list&, const ipr::Type&,
                       const ipr::Expr_list&);
         Conditional* make_conditional(const ipr::Expr&, const ipr::Expr&,
                                       const ipr::Expr&);

         Rname* rname_for_next_param(const Mapping&, const ipr::Type&);

         Mapping* make_mapping(const ipr::Region&, const ipr::Type&, int = 0);

      private:
         util::string::arena string_pool;
         util::rb_tree::container<impl::String> strings;

         // Language linkage nodes.
         util::rb_tree::container<impl::Linkage> linkages;

         util::rb_tree::container<impl::Conversion> convs;
         util::rb_tree::container<impl::Ctor_name> ctors;
         util::rb_tree::container<impl::Dtor_name> dtors;
         util::rb_tree::container<impl::Identifier> ids;
         util::rb_tree::container<impl::Literal> lits;
         util::rb_tree::container<impl::Operator> ops;
         util::rb_tree::container<impl::Rname> rnames;
         util::rb_tree::container<impl::Scope_ref> scope_refs;
         util::rb_tree::container<Template_id> template_ids;
         util::rb_tree::container<impl::Type_id> typeids;
         // >>>> Yuriy Solodkyy: 2008/07/10 
         // Made sizeof(T) and typeid(T) unified.
         util::rb_tree::container<impl::Type_sizeof> tsizeofs;
         util::rb_tree::container<impl::Type_typeid> ttypeids;
         // <<<< Yuriy Solodkyy: 2008/07/10 

         util::slist<impl::Phantom> phantoms;
         
         util::slist<impl::Address> addresses;
         util::slist<impl::Annotation> annotations;
         util::slist<impl::Array_delete> array_deletes;
         util::slist<impl::Complement> complements;
         util::slist<impl::Delete> deletes;
         util::slist<impl::Deref> derefs;
         util::slist<impl::Expr_list> xlists;
         util::slist<impl::Expr_sizeof> xsizeofs;
         util::slist<impl::Expr_typeid> xtypeids;
         util::slist<impl::Id_expr> id_exprs;
         util::slist<impl::Initializer_list> init_lists;
         util::slist<impl::Not> nots;
         util::slist<impl::Pre_increment> pre_increments;
         util::slist<impl::Pre_decrement> pre_decrements;
         util::slist<impl::Post_increment> post_increments;
         util::slist<impl::Post_decrement> post_decrements;
         util::slist<impl::Paren_expr> parens;
         util::slist<impl::Throw> throws;
         util::slist<impl::Unary_minus> unary_minuses;
         util::slist<impl::Unary_plus> unary_pluses;

         util::slist<impl::And> ands;
         util::slist<impl::Array_ref> array_refs;
         util::slist<impl::Arrow> arrows;
         util::slist<impl::Arrow_star> arrow_stars;
         util::slist<impl::Assign> assigns;
         util::slist<impl::Bitand> bitands;
         util::slist<impl::Bitand_assign> bitand_assigns;
         util::slist<impl::Bitor> bitors;
         util::slist<impl::Bitor_assign> bitor_assigns;
         util::slist<impl::Bitxor> bitxors;
         util::slist<impl::Bitxor_assign> bitxor_assigns;
         util::slist<impl::Cast> casts;
         util::slist<impl::Call> calls;
         util::slist<impl::Comma> commas;
         util::slist<impl::Const_cast> ccasts;
         util::slist<impl::Datum> data;
         util::slist<impl::Div> divs;
         util::slist<impl::Div_assign> div_assigns;
         util::slist<impl::Dot> dots;
         util::slist<impl::Dot_star> dot_stars;
         util::slist<impl::Dynamic_cast> dcasts;
         util::slist<impl::Equal> equals;
         util::slist<impl::Greater> greaters;
         util::slist<impl::Greater_equal> greater_equals;
         util::slist<impl::Less> lesses;
         util::slist<impl::Less_equal> less_equals;
         util::slist<impl::Lshift> lshifts;
         util::slist<impl::Lshift_assign> lshift_assigns;
         util::slist<impl::Member_init> member_inits;
         util::slist<impl::Minus> minuses;
         util::slist<impl::Minus_assign> minus_assigns;
         util::slist<impl::Modulo> modulos;
         util::slist<impl::Modulo_assign> modulo_assigns;
         util::slist<impl::Mul> muls;
         util::slist<impl::Mul_assign> mul_assigns;
         util::slist<impl::Not_equal> not_equals;
         util::slist<impl::Or> ors;
         util::slist<impl::Plus> pluses;
         util::slist<impl::Plus_assign> plus_assigns;
         util::slist<impl::Reinterpret_cast> rcasts;
         util::slist<impl::Rshift> rshifts;
         util::slist<impl::Rshift_assign> rshift_assigns;
         util::slist<impl::Static_cast> scasts;
         
         util::slist<impl::New> news;
         util::slist<impl::Conditional> conds;
         util::slist<impl::Mapping> mappings;

         const ipr::String& get_string(const char*, int);
      };

      
      struct Named_map : impl::Decl<ipr::Named_map> {
         const ipr::Udt* member_of;
         impl::Mapping* init;
         const ipr::Region* lexreg;
         impl::Expr_list args;

         Named_map();

         const ipr::Named_map& primary_named_map() const;
         const ipr::Sequence<ipr::Decl>& specializations() const;
         const ipr::Mapping& mapping() const;
         const ipr::Expr& initializer() const;
         bool has_initializer() const;
         const ipr::Region& lexical_region() const;
      };

      template<>
      struct traits<ipr::Named_map> {
         typedef impl::Named_map rep;
      };
      
      template<class Interface>
      struct decl_factory {
         util::slist<decl_rep<Interface> > decls;
         util::slist<master_decl_data<Interface> > master_info;

         // We have gotten an overload-set for a name, and we are about
         // to enter the first declaration for that name with the type T.
         decl_rep<Interface>* declare(Overload* ovl, const ipr::Type& t)
         {
            // Grab bookkeeping memory for this master declaration.
            master_decl_data<Interface>* data = master_info.push_back(ovl, t);
            // The actual representation for the declaration points back
            // to the master declaration bookkeeping store.
            decl_rep<Interface>* master = decls.push_back(data);
            // Inform the overload-set that we have a new master declaration.
            ovl->push_back(data);

            return master;
         }
         
         decl_rep<Interface>* redeclare(overload_entry* decl)
         {
            return decls.push_back
               (static_cast<master_decl_data<Interface>*>(decl));
         }
      };


      struct Alias : impl::Decl<ipr::Alias> {
         const ipr::Expr* aliasee;
         const ipr::Region* lexreg;

         Alias();
         const ipr::Expr& initializer() const;
         bool has_initializer() const;
         const ipr::Region& lexical_region() const;
      };

      template<>
      struct traits<ipr::Alias> {
         typedef impl::Alias rep; 
      };
      
      struct Var : impl::Decl<ipr::Var> {
         const ipr::Expr* init;
         const ipr::Region* lexreg;

         Var();

         // Override ipr::Decl::has_initializer.
         bool has_initializer() const;

         // Override ipr::Decl::initializer.
         const ipr::Expr& initializer() const;

         // Override ipr::Decl::lexical_region.
         const ipr::Region& lexical_region() const;
      };

      template<>
      struct traits<ipr::Var> {
         typedef impl::Var rep;
      };

      // FIXME: Field should use unique_decl, not impl::Decl.
      struct Field : impl::Decl<ipr::Field> {
         const ipr::Udt* member_of;
         const ipr::Expr* init;

         Field();

         // Override ipr::Field::membership.
         const ipr::Udt& membership() const;

         // Override ipr::Decl::has_initializer.
         bool has_initializer() const;

         // Override ipr::Decl::lexical_region.  Which is the
         // same as home_region for a Field.  Note that fields
         // are always nonstatic data members.
         const ipr::Region& lexical_region() const;
         const ipr::Region& home_region() const;

         // Override ipr::Decl::initializer.
         const ipr::Expr& initializer() const;
      };

      template<>
      struct traits<ipr::Field> {
         typedef impl::Field rep;
      };

      // FIXME: Bitfield should use unique_decl, not impl::Decl
      struct Bitfield : impl::Decl<ipr::Bitfield> {
         const ipr::Expr* length;
         const ipr::Udt* member_of;
         const ipr::Expr* init;

         Bitfield();

         // Override Bitfield::precision.
         const ipr::Expr& precision() const;

         const ipr::Region& lexical_region() const;
         const ipr::Region& home_region() const;

         // Override Bitfield::membership.
         const ipr::Udt& membership() const;

         // Override ipr::Decl::has_initializer.
         bool has_initializer() const;

         // Override ipr::Decl::initializer.
         const ipr::Expr& initializer() const;

      };

      template<>
      struct traits<ipr::Bitfield> {
         typedef impl::Bitfield rep;
      };

      struct Typedecl : impl::Decl<ipr::Typedecl> {
         const ipr::Type* init;
         const ipr::Udt* member_of;
         const ipr::Region* lexreg;

         Typedecl();

         // Override ipr::Typedecl::membership.
         const ipr::Udt& membership() const;

         // Override ipr::Decl:initializer
         const ipr::Expr& initializer() const;

         // Override ipr::Decl::has_initializer.
         bool has_initializer() const;

         const ipr::Region& lexical_region() const;
      };

      template<>
      struct traits<ipr::Typedecl> {
         typedef impl::Typedecl rep;
      };

      struct Fundecl : impl::Decl<ipr::Fundecl> {
         const ipr::Udt* member_of;
         impl::Mapping* init;
         const ipr::Region* lexreg;

         Fundecl();
         
         // Override Fundecl::mapping.
         const ipr::Mapping& mapping() const;
         
         // Override Fundecl::membership.
         const ipr::Udt& membership() const;

         // Override ipr::Decl::initializer;
         const ipr::Expr& initializer() const;

         // Override ipr::Decl::has_initializer.
         bool has_initializer() const;

         const ipr::Region& lexical_region() const;
      };

      template<>
      struct traits<ipr::Fundecl> {
         typedef impl::Fundecl rep;
      };

      
      // A heterogeneous scope is a sequence of declarations of
      // almost of kinds.  The omitted kinds being parameters,
      // base-class subobjects and enumerators.   Those form
      // a homogeneous scope, implemented by homogeneous_scope.
      
      struct Scope : impl::Node<ipr::Scope> {
         Scope(const ipr::Region&, const ipr::Type&);
         
         const ipr::Type& type() const;
         const ipr::Sequence<ipr::Decl>& members() const;
         const ipr::Overload& operator[](const ipr::Name&) const;

         impl::Alias* make_alias(const ipr::Name&, const ipr::Expr&);
         impl::Var* make_var(const ipr::Name&, const ipr::Type&);
         impl::Field* make_field(const ipr::Name&, const ipr::Type&);
         impl::Bitfield* make_bitfield(const ipr::Name&, const ipr::Type&);
         impl::Typedecl* make_typedecl(const ipr::Name&, const ipr::Type&);
         impl::Fundecl* make_fundecl(const ipr::Name&, const ipr::Function&);
         impl::Named_map* make_primary_map(const ipr::Name&,
                                           const ipr::Template&);
         impl::Named_map* make_secondary_map(const ipr::Name&,
                                             const ipr::Template&);
      
      private:
         const ipr::Region& region;
         util::rb_tree::container<impl::Overload> overloads;
         typed_sequence<decl_sequence> decls;
         empty_overload missing;

         decl_factory<ipr::Alias> aliases;
         decl_factory<ipr::Var> vars;
         decl_factory<ipr::Field> fields;
         decl_factory<ipr::Bitfield> bitfields;
         decl_factory<ipr::Fundecl> fundecls;
         decl_factory<ipr::Typedecl> typedecls;

         decl_factory<ipr::Named_map> primary_maps;
         decl_factory<ipr::Named_map> secondary_maps;

         template<class T> inline void add_member(T*);
      };


      // A heterogeneous region is a region of program text that
      // contains heterogeneous scope (as defined above).

      struct Region : impl::Node<ipr::Region> {
         typedef ipr::Region::Location_span location_span;
         const ipr::Region* parent;
         location_span extent;
         const ipr::Expr* owned_by;
         impl::Scope scope;

         const ipr::Region& enclosing() const;
         const ipr::Scope& bindings() const;
         const location_span& span() const;
         const ipr::Expr& owner() const;

         impl::Region* make_subregion();

         // Convenient functions, forwarding to those of SCOPE.
         impl::Alias* declare_alias(const ipr::Name& n, const ipr::Type& t)
         {
            return scope.make_alias(n, t);
         }

         impl::Var* declare_var(const ipr::Name& n, const ipr::Type& t)
         {
            return scope.make_var(n, t);
         }

         impl::Field* declare_field(const ipr::Name& n, const ipr::Type& t)
         {
            return scope.make_field(n, t);
         }

         impl::Bitfield* declare_bitfield(const ipr::Name& n,
                                          const ipr::Type& t)
         {
            return scope.make_bitfield(n, t);
         }
         

         Typedecl* declare_type(const ipr::Name& n, const ipr::Type& t)
         {
            return scope.make_typedecl(n, t);
         }

         Fundecl* declare_fun(const ipr::Name& n, const ipr::Function& t)
         {
            return scope.make_fundecl(n, t);
         }

         Named_map* declare_primary_map(const ipr::Name& n,
                                        const ipr::Template& t)
         {
            return scope.make_primary_map(n, t);
         }

         Named_map* declare_secondary_map(const ipr::Name& n,
                                          const ipr::Template& t)
         {
            return scope.make_secondary_map(n, t);
         }

         Region(const ipr::Region*, const ipr::Type&);

      private:
         util::slist<Region> subregions;
      };


      // Implement common operations for user-defined types.  The case
      // of enums is handled separately because its body is a
      // homogeneous region.

      template<class Interface>
      struct Udt : impl::Type<Interface> {
         Region body;
         Udt(const ipr::Region* pr, const ipr::Type& t) : body(pr, t)
         {
            this->constraint = &t;
            body.owned_by = this;
         }
         
         // Override Udt::region.
         const ipr::Region& region() const { return body; }
         
         impl::Alias*
         declare_alias(const ipr::Name& n, const ipr::Type& t)
         {
            impl::Alias* alias = body.declare_alias(n, t);
            //alias->member_of = this; // FIX: Add member_of to Alias
            return alias;
         }

         impl::Field*
         declare_field(const ipr::Name& n, const ipr::Type& t)
         {
            impl::Field* field = body.declare_field(n, t);
            field->member_of = this;
            return field;
         }
         
         impl::Bitfield*
         declare_bitfield(const ipr::Name& n, const ipr::Type& t)
         {
            impl::Bitfield* field = body.declare_bitfield(n, t);
            field->member_of = this;
            return field;
         }
         
         impl::Var*
         declare_var(const ipr::Name& n, const ipr::Type& t)
         {
            impl::Var* var = body.declare_var(n, t);
            return var;
         }
         
         impl::Typedecl*
         declare_type(const ipr::Name& n, const ipr::Type& t)
         {
            impl::Typedecl* typedecl = body.declare_type(n, t);
            typedecl->member_of = this;
            return typedecl;
         }
         
         impl::Fundecl*
         declare_fun(const ipr::Name& n, const ipr::Function& t)
         {
            impl::Fundecl* fundecl = body.declare_fun(n, t);
            fundecl->member_of = this;
            return fundecl;
         }
         
         impl::Named_map*
         declare_primary_map(const ipr::Name& n, const ipr::Template& t)
         {
            impl::Named_map* map = body.declare_primary_map(n, t);
            map->member_of = this;
            return map;
         }

         impl::Named_map*
         declare_secondary_map(const ipr::Name& n, const ipr::Template& t)
         {
            impl::Named_map* map = body.declare_secondary_map(n, t);
            map->member_of = this;
            return map;
         }
      };

      struct Enum : impl::Type<ipr::Enum> {
         homogeneous_region<ipr::Enumerator> body;

         const ipr::Region& region() const;
         const Sequence<ipr::Enumerator>& members() const;

         impl::Enumerator* add_member(const ipr::Name&);
         
         explicit Enum(const ipr::Region&, const ipr::Type&);
      };

      typedef Udt<ipr::Union> Union;
      typedef Udt<ipr::Namespace> Namespace;

      struct Class : impl::Udt<ipr::Class> {
         homogeneous_region<ipr::Base_type> base_subobjects;
         
         explicit Class(const ipr::Region&, const ipr::Type&);

         // Override Class::bases.
         const ipr::Sequence<ipr::Base_type>& bases() const;

         impl::Base_type* declare_base(const ipr::Type&);

      };



      // This class is responsible for creating nodes that
      // represent types.  It is responsible for the storage
      // management that is implied.  Notice that the type nodes
      // created by this class may need additional processing such
      // as setting their types (as expressions) and their names.

      struct type_factory {
         // Build an IPR node for an expression that denotes a type.
         // The linkage, if not specified, is assumed to be C++.
         impl::As_type* make_as_type(const ipr::Expr&);
         impl::As_type* make_as_type(const ipr::Expr&, const ipr::Linkage&);

         impl::Array* make_array(const ipr::Type&, const ipr::Expr&);
         impl::Qualified* make_qualified(ipr::Type::Qualifier,
                                         const ipr::Type&);
         impl::Decltype* make_decltype(const ipr::Expr&);
         impl::Function* make_function(const ipr::Product&, const ipr::Type&,
                                       const ipr::Sum&, const ipr::Linkage&);
         impl::Pointer* make_pointer(const ipr::Type&);
         impl::Product* make_product(const ipr::Sequence<ipr::Type>&);
         impl::Ptr_to_member* make_ptr_to_member(const ipr::Type&,
                                                 const ipr::Type&);
         impl::Reference* make_reference(const ipr::Type&);
         impl::Rvalue_reference* make_rvalue_reference(const ipr::Type&);
         impl::Sum* make_sum(const ipr::Sequence<ipr::Type>&);
         impl::Template* make_template(const ipr::Product&, const ipr::Type&);

         impl::Enum* make_enum(const ipr::Region&, const ipr::Type&);
         impl::Class* make_class(const ipr::Region&, const ipr::Type&);
         impl::Union* make_union(const ipr::Region&, const ipr::Type&);
         impl::Namespace* make_namespace(const ipr::Region*, const ipr::Type&);
            
      private:
         util::rb_tree::container<impl::Array> arrays;
         util::rb_tree::container<impl::Decltype> decltypes;
         util::rb_tree::container<impl::As_type> type_refs;
         util::rb_tree::container<impl::Function> functions;
         util::rb_tree::container<impl::Pointer> pointers;
         util::rb_tree::container<impl::Product> products;
         util::rb_tree::container<impl::Ptr_to_member> member_ptrs;
         util::rb_tree::container<impl::Qualified> qualifieds;
         util::rb_tree::container<impl::Reference> references;
         util::rb_tree::container<impl::Rvalue_reference> refrefs;
         util::rb_tree::container<impl::Sum> sums;
         util::rb_tree::container<impl::Template> templates;
         util::slist<impl::Enum> enums;
         util::slist<impl::Class> classes;
         util::slist<impl::Union> unions;
         util::slist<impl::Namespace> namespaces;
      };


      // ----------------------------------
      // -- Implementation of statements --
      // ----------------------------------

      typedef Binary<Stmt<Expr<ipr::Ctor_body> > > Ctor_body;
      typedef type_from_second<Stmt<Node<ipr::Do> > > Do;
      typedef type_from_operand<Stmt<Node<ipr::Expr_stmt> > > Expr_stmt;
      typedef type_from_operand<Stmt<Node<ipr::Empty_stmt> > > Empty_stmt;
      typedef type_from_operand<Stmt<Node<ipr::Goto> > > Goto;
      typedef type_from_second<Stmt<Node<ipr::Handler> > > Handler;
      typedef type_from_second<Stmt<Node<ipr::If_then> > > If_then;
      typedef Ternary<Stmt<Expr<ipr::If_then_else> > > If_then_else;
      typedef type_from_second<Stmt<Node<ipr::Labeled_stmt> > > Labeled_stmt;
      typedef type_from_operand<Stmt<Node<ipr::Return> > > Return;
      typedef type_from_second<Stmt<Node<ipr::Switch> > > Switch;
      typedef type_from_second<Stmt<Node<ipr::While> > > While;

      // A Block holds a heterogeneous region, suitable for
      // recording the set of declarations appearing in that
      // block.  It also holds a sequence of handlers, when the
      // block actually represents a C++ try-block.

      struct Block : impl::Stmt<Node<ipr::Block> > {
         Region region;
         ref_sequence<ipr::Stmt> stmt_seq;
         ref_sequence<ipr::Handler> handler_seq;
         
         Block(const ipr::Region&, const ipr::Type&);

         // Override ipr::Expr::type.
         const ipr::Type& type() const;
         
         // Override ipr::Block::members.
         const ipr::Scope& members() const;
         
         // Override ipr::Block::body.
         const ipr::Sequence<ipr::Stmt>& body() const;
         
         // Override ipr::Block::handlers.
         const ipr::Sequence<ipr::Handler>& handlers() const;

         // The scope of declarations in this block
         Scope* scope() { return &region.scope; }

         void add_stmt(const ipr::Stmt* s)
         {
            stmt_seq.push_back(s);
         }

         void add_handler(const ipr::Handler* h)
         {
            handler_seq.push_back(h);
         }
      };


      // A for-statement node in its most general form is a quaternry
      // expresion; for flexibility, it is made in a way that
      // supports settings of its components after construction.

      struct For : Stmt<Expr<ipr::For> > {
         const ipr::Expr* init;
         const ipr::Expr* cond;
         const ipr::Expr* inc;
         const ipr::Stmt* stmt;

         For();

         const ipr::Type& type() const; // override ipr::Expr::type.
         
         // Override ipr::For::initializer.
         const ipr::Expr& initializer() const;
         
         // Override ipr::For::condition.
         const ipr::Expr& condition() const;
         
         // Override ipr::For::increment.
         const ipr::Expr& increment() const;
         
         // Override ipr::For::body.
         const ipr::Stmt& body() const;
      };

      struct For_in : Stmt<Expr<ipr::For_in> > {
         const ipr::Var* var;
         const ipr::Expr* seq;
         const ipr::Stmt* stmt;

         For_in();

         const ipr::Type& type() const; // override ipr::Expr::type.
         
         // Override ipr::For_in::variable.
         const ipr::Var& variable() const;
         
         // Override ipr::For_in::sequence.
         const ipr::Expr& sequence() const;
         
         // Override ipr::For_in::body.
         const ipr::Stmt& body() const;
      };


      // A Break node can record the selction- of iteration-statement it
      // transfers control out of.

      struct Break : Stmt<Node<ipr::Break> > {
         const ipr::Stmt* stmt;

         Break();
         const ipr::Type& type() const; // override ipr::Expr::type.
         const ipr::Stmt& from() const; // Override ipr::Break::from.
      };


      // Like a Break, a Continue node can refer back to the
      // iteration-statement containing it.

      struct Continue : Stmt<Node<ipr::Continue> > {
         const ipr::Stmt* stmt;

         Continue();
         const ipr::Type& type() const; // override ipr::Expr::type.
         const ipr::Stmt& iteration() const; // override Continue::iteration.
      };


      // This factory class takes on the implementation burden of
      // allocating storage for statement nodes and their constructions.

      struct stmt_factory : expr_factory {
         impl::Break* make_break();
         impl::Continue* make_continue();
         impl::Empty_stmt* make_empty_stmt();
         impl::Block* make_block(const ipr::Region&, const ipr::Type&);
         impl::Ctor_body* make_ctor_body(const ipr::Expr_list&,
                                         const ipr::Block&);
         impl::Expr_stmt* make_expr_stmt(const ipr::Expr&);
         impl::Goto* make_goto(const ipr::Expr&);
         impl::Return* make_return(const ipr::Expr&);
         impl::Do* make_do(const ipr::Stmt&, const ipr::Expr&);
         impl::If_then* make_if_then(const ipr::Expr&, const ipr::Stmt&);
         impl::Switch* make_switch(const ipr::Expr&, const ipr::Stmt&);
         impl::Handler* make_handler(const ipr::Decl&, const ipr::Block&);
         impl::Labeled_stmt* make_labeled_stmt(const ipr::Expr&,
                                               const ipr::Stmt&);
         impl::While* make_while(const ipr::Expr&, const ipr::Stmt&);
         impl::If_then_else* make_if_then_else(const ipr::Expr&,
                                               const ipr::Stmt&,
                                               const ipr::Stmt&);
         impl::For* make_for();
         impl::For_in* make_for_in();

      protected:
         util::slist<impl::Break> breaks;
         util::slist<impl::Continue> continues;
         util::slist<impl::Empty_stmt> empty_stmts;
         util::slist<impl::Block> blocks;
         util::slist<impl::Expr_stmt> expr_stmts;
         util::slist<impl::Goto> gotos;
         util::slist<impl::Return> returns;
         util::slist<impl::Ctor_body> ctor_bodies;
         util::slist<impl::Do> dos;
         util::slist<impl::If_then> ifs;
         util::slist<impl::Handler> handlers;
         util::slist<impl::Labeled_stmt> labeled_stmts;
         util::slist<impl::Switch> switches;
         util::slist<impl::While> whiles;
         util::slist<impl::If_then_else> ifelses;
         util::slist<impl::For> fors;
         util::slist<impl::For_in> for_ins;
      };


      // This template will be instantiated to implementations
      // of several builtin type singletons.  It does not reuse
      // implementations of Type, and Binary, to save space since
      // most data needed by those implementations are shared.
      template<class T>
      struct Builtin : impl::Expr<T> {
         typedef typename T::Arg1_type Arg1_type;
         typedef typename T::Arg2_type Arg2_type;
         Builtin(const ipr::Name& n, Arg2_type l, const ipr::Type& t)
               : impl::Expr<T>(&t), id(n), link(l) { }

         const ipr::Name& name() const { return id; }
         Arg1_type first() const { return id; }
         Arg2_type second() const { return link; }

      private:
         const ipr::Name& id;
         Arg2_type link;
      };
      

      struct Unit : impl::Node<ipr::Unit>, stmt_factory {
         Unit();
         ~Unit();

         const ipr::Linkage& get_cxx_linkage() const;
         const ipr::Linkage& get_c_linkage() const;
         
         const ipr::Global_scope& get_global_scope() const;

         const ipr::Identifier& get_identifier(const char*);
         const ipr::Identifier& get_identifier(const std::string&);
         const ipr::Identifier& get_identifier(const ipr::String&);
         
         const ipr::Operator& get_operator(const char*);
         const ipr::Operator& get_operator(const std::string&);
         const ipr::Operator& get_operator(const ipr::String&);

         const ipr::Ctor_name& get_ctor_name(const ipr::Type&);
         const ipr::Dtor_name& get_dtor_name(const ipr::Type&);

         const ipr::Conversion& get_conversion(const ipr::Type&);

         const ipr::Scope_ref& get_scope_ref(const ipr::Expr&,
                                             const ipr::Expr&);

         const ipr::Template_id& get_template_id(const ipr::Name&,
                                                 const ipr::Expr_list&);

         const ipr::Literal& get_literal(const ipr::Type&, const char*);
         const ipr::Literal& get_literal(const ipr::Type&, const std::string&);
         const ipr::Literal& get_literal(const ipr::Type&, const ipr::String&);
         
         const ipr::Void& get_void() const;
         const ipr::Bool& get_bool() const;

         const ipr::Char& get_char() const;
         const ipr::sChar& get_schar() const;
         const ipr::uChar& get_uchar() const;
         const ipr::Wchar_t& get_wchar_t() const;

         const ipr::Short& get_short() const;
         const ipr::uShort& get_ushort() const;

         const ipr::Int& get_int() const;
         const ipr::uInt& get_uint() const;

         const ipr::Long& get_long() const;
         const ipr::uLong& get_ulong() const;

         const ipr::Long_long& get_long_long() const;
         const ipr::uLong_long& get_ulong_long() const;

         const ipr::Float& get_float() const;
         const ipr::Double& get_double() const;
         const ipr::Long_double& get_long_double() const;

         const ipr::Ellipsis& get_ellipsis() const;

         const ipr::Type& get_typename() const;
         const ipr::Type& get_class() const;
         const ipr::Type& get_union() const;
         const ipr::Type& get_enum() const;
         const ipr::Type& get_namespace() const;

         const ipr::Array& get_array(const ipr::Type&, const ipr::Expr&);

         const ipr::As_type& get_as_type(const ipr::Expr&);
         const ipr::As_type& get_as_type(const ipr::Expr&,
                                         const ipr::Linkage&);

         const ipr::Decltype& get_decltype(const ipr::Expr&);

         const ipr::Function& get_function(const ipr::Product&,
                                           const ipr::Type&, const ipr::Sum&);
         const ipr::Function& get_function(const ipr::Product&,
                                           const ipr::Type&);
         const ipr::Function& get_function(const ipr::Product&,
                                           const ipr::Type&,
                                           const ipr::Sum&,
                                           const ipr::Linkage&);
         const ipr::Function& get_function(const ipr::Product&,
                                           const ipr::Type&,
                                           const ipr::Linkage&);

         const ipr::Pointer& get_pointer(const ipr::Type&);

         const ipr::Product& get_product(const ref_sequence<ipr::Type>&);

         const ipr::Ptr_to_member& get_ptr_to_member(const ipr::Type&,
                                                     const ipr::Type&);

         const ipr::Reference& get_reference(const ipr::Type&);
         const ipr::Rvalue_reference& get_rvalue_reference(const ipr::Type&);

         const ipr::Qualified& get_qualified(ipr::Type::Qualifier,
                                             const ipr::Type&);

         const ipr::Sum& get_sum(const ref_sequence<ipr::Type>&);

         const ipr::Template& get_template(const ipr::Product&,
                                           const ipr::Type&);

         
         Region* global_region() { return &global_ns.body; }
         
         Scope* global_scope() { return &global_ns.body.scope; }

         impl::Mapping* make_mapping(const ipr::Region&);
         impl::Parameter* make_parameter(const ipr::Name&, const ipr::Type&,
                                         impl::Mapping&);

         impl::Class* make_class(const ipr::Region&);
         impl::Enum* make_enum(const ipr::Region&);
         impl::Namespace* make_namespace(const ipr::Region&);
         impl::Union* make_union(const ipr::Region&);

         int make_fileindex(const ipr::String&);
         const ipr::String& to_filename(int) const;

      private:
         void record_builtin_type(const ipr::As_type&);

         typedef util::slist<impl::String> Filemap;


         Filemap filemap;
         type_factory types;
         util::rb_tree::container<ref_sequence<ipr::Expr> > expr_seqs;
         util::rb_tree::container<ref_sequence<ipr::Type> > type_seqs;
         util::rb_tree::container<node_ref<ipr::As_type> > builtin_map;

         const impl::Builtin<ipr::As_type> anytype;
         const impl::Builtin<ipr::As_type> classtype;
         const impl::Builtin<ipr::As_type> uniontype;
         const impl::Builtin<ipr::As_type> enumtype;
         const impl::Builtin<ipr::As_type> namespacetype;
         const impl::Builtin<ipr::Void> voidtype;
         const impl::Builtin<ipr::Bool> booltype;
         const impl::Builtin<ipr::Char> chartype;
         const impl::Builtin<ipr::sChar> schartype;
         const impl::Builtin<ipr::uChar> uchartype;
         const impl::Builtin<ipr::Wchar_t> wchar_ttype;
         const impl::Builtin<ipr::Short> shorttype;
         const impl::Builtin<ipr::uShort> ushorttype;
         const impl::Builtin<ipr::Int> inttype;
         const impl::Builtin<ipr::uInt> uinttype;
         const impl::Builtin<ipr::Long> longtype;
         const impl::Builtin<ipr::uLong> ulongtype;
         const impl::Builtin<ipr::Long_long> longlongtype;
         const impl::Builtin<ipr::uLong_long> ulonglongtype;
         const impl::Builtin<ipr::Float> floattype;
         const impl::Builtin<ipr::Double> doubletype;
         const impl::Builtin<ipr::Long_double> longdoubletype;
         const impl::Builtin<ipr::Ellipsis> ellipsistype;

         template<class T> T* finish_type(T*);

      public:
         impl::Udt<ipr::Global_scope> global_ns;
      };
   }
}

#endif