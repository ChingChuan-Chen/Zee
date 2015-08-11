/*
File: include/sparse_matrix.h

This file is part of the Zee partitioning framework

Copyright (C) 2015 Jan-Willem Buurlage <janwillembuurlage@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License (LGPL)
as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.
*/

#pragma once

#include <iterator>
#include <type_traits>
#include <memory>
#include <vector>

namespace Zee {

using std::vector;

template <typename TVal, typename TIdx>
class Triplet;

//-----------------------------------------------------------------------------

template <typename TVal, typename TIdx, bool const_iter>
class StorageIterator;

template <typename TVal, typename TIdx, class ItTraits>
class DSparseStorage;

//-----------------------------------------------------------------------------

template <typename TVal, typename TIdx>
struct TraitsTriplets;

template <typename TVal, typename TIdx,
         class ItTraits = TraitsTriplets<TVal, TIdx>>
class StorageTriplets;

//-----------------------------------------------------------------------------

template <typename TVal, typename TIdx>
struct TraitsCCS;

template <typename TVal, typename TIdx>
struct TraitsRCS;

template <typename TVal, typename TIdx, class ItTraits>
class DSparseStorageCompressed;

template <typename TVal, typename TIdx, class ItTraits>
using ColumnCompressedStorage =
    DSparseStorageCompressed<TVal, TIdx, TraitsCCS<TVal, TIdx>>;

template <typename TVal, typename TIdx, class ItTraits>
using RowCompressedStorage =
    DSparseStorageCompressed<TVal, TIdx, TraitsRCS<TVal, TIdx>>;

//-----------------------------------------------------------------------------
// Base iterator
//-----------------------------------------------------------------------------

template <typename TVal, typename TIdx, bool const_iter = true>
class StorageIterator:
    public std::iterator<
        std::bidirectional_iterator_tag,
        Triplet<TVal, TIdx>>
{
    // TODO: Move whatever possible here from below
};

//-----------------------------------------------------------------------------
// Triplet Storage
//-----------------------------------------------------------------------------

/** FIXME: There should be a base class for storage iteration */
template <typename TVal, typename TIdx, bool const_iter = true>
class StorageIteratorTriplets:
    public StorageIterator<TVal, TIdx, const_iter>
{
    public:
        // Define whether our data type is constant
        using StoragePointer = typename std::conditional<const_iter,
                const StorageTriplets<TVal, TIdx>*,
                StorageTriplets<TVal, TIdx>*>::type;

        using TripletReference = typename std::conditional<const_iter,
                const Triplet<TVal, TIdx>&,
                Triplet<TVal, TIdx>&>::type;

        /** Default constructor */
        StorageIteratorTriplets(StoragePointer storage, TIdx i) :
            _storage(storage),
            _i(i) { }

        /** Copy constructor (const <-> regular conversion) */
        StorageIteratorTriplets(
                const StorageIteratorTriplets<TVal, TIdx, false>& other) :
            _storage(other._storage),
            _i(other._i)  { }

        StorageIteratorTriplets& operator--(int)
        {
            // copy iterator, decrease this and return old copy
            const StorageIteratorTriplets old(*this);
            --(*this);
            return old;
        }

        StorageIteratorTriplets& operator++(int)
        {
            // copy iterator, increase this and return old copy
            const StorageIteratorTriplets old(*this);
            ++(*this);
            return old;
        }

        bool operator== (const StorageIteratorTriplets& other) const
        {
            return (_i == other._i);
        }

        /** Comparison operator for not equal
          * @see operator==(const dual_iterator& other) const */
        bool operator!= (const StorageIteratorTriplets& other) const
        {
            return !(*this == other);
        }

        TripletReference operator*()
        {
            return _storage->_triplets[_i];
        }

        StorageIteratorTriplets& operator--() {
            _i--;
            return *this;
        }

        StorageIteratorTriplets& operator++()
        {
            _i++;
            return *this;
        }

       /* now the copy constructor can access _storage for 
        * conversion */
       friend class StorageIteratorTriplets<TVal, TIdx, true>; 

    private:
        StoragePointer _storage;
        TIdx _i;
};


// We put traits in separate struct to avoid deadly diamond
template <typename TVal, typename TIdx>
struct TraitsTriplets
{
        typedef StorageIteratorTriplets<TVal, TIdx, false>
            iterator;
        typedef StorageIteratorTriplets<TVal, TIdx, true>
            const_iterator;
};

template <typename TVal, typename TIdx, class ItTraits>
class StorageTriplets :
    public DSparseStorage<TVal, TIdx, ItTraits>
{
    public:
        /** We define iterators and constant iterators for getting out
          * triplets */
        typedef ItTraits it_traits;
        typedef typename ItTraits::iterator iterator;
        typedef typename ItTraits::const_iterator const_iterator;

        iterator begin() override
        {
            return iterator(this, 0);
        }

        iterator end() override
        {
            return iterator(this, _triplets.size());
        }

        const_iterator cbegin() const override
        {
            return const_iterator(this, 0);
        }

        const_iterator cend() const override
        {
            return const_iterator(this, _triplets.size());
        }

        StorageTriplets() = default;
        ~StorageTriplets() = default;

        Triplet<TVal, TIdx> popElement(TIdx element) override
        {
            // FIXME: does this get copied? want by ref, move?
            Triplet<TVal, TIdx> trip = _triplets[element];
            _triplets.erase(_triplets.begin() + element);
            return trip;
        }

        void pushTriplet(Triplet<TVal, TIdx> t) override
        {
            // FIXME: does this get copied? want by ref, move?
            _triplets.push_back(t);
        }

        virtual Triplet<TVal, TIdx> getElement(TIdx i) const override {
            return _triplets[i];
        }

        virtual TIdx size() const override {
            return _triplets.size();
        }

        // FIXME: move to getter?! although iterators truly are 'friends'
        friend iterator;
        friend const_iterator;

    private:
        // FIXME: proper wasy to store this (reference?)
        vector<Triplet<TVal, TIdx>> _triplets;
};

//-----------------------------------------------------------------------------
// Compressed Storage
//-----------------------------------------------------------------------------

// TODO: implement
// Note: think about 'frame' table, store complete triplet every 1000 or so
// elements allowing to binary search for elements (amortized constant time?)

//-----------------------------------------------------------------------------
// Base Storage
//-----------------------------------------------------------------------------

/** A pure abstract base class which serves as a storage concept.
 * Allows iteration of the underlying matrix elements. Is constructed by feeding
 * triplets. */
template <typename TVal, typename TIdx, class ItTraits>
class DSparseStorage
{
    public:
        typedef typename ItTraits::iterator iterator;
        typedef typename ItTraits::const_iterator const_iterator;

        DSparseStorage() = default;
        virtual ~DSparseStorage() = default;

        /** Removes the triplet at index element and returns it. */
        virtual Triplet<TVal, TIdx> popElement(TIdx element) = 0;

        /** Adds the triplet t to the storage */
        virtual void pushTriplet(Triplet<TVal, TIdx> t) = 0;

        /** The number of matrix elements stored. */
        virtual TIdx size() const = 0;

        /** Obtain the i-th element as a triplet.
         *  @note Complexity depends on implementation.
         *  @return triplet at index i */
        virtual Triplet<TVal, TIdx> getElement(TIdx i) const = 0;

        /** We define iterators and constant iterators for iterating over
          * triplets */
        virtual iterator begin() = 0;
        virtual iterator end() = 0;
        virtual const_iterator cbegin() const = 0;
        virtual const_iterator cend() const = 0;
};

} // namespace Zee
