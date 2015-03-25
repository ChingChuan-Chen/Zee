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

#include <cassert>

#include <iostream>
using std::cout;
using std::endl;

#include <cstdint>

#include <vector>
using std::vector;

#include <matrix/base.hpp>
#include <matrix/dense.hpp>
#include <matrix/storage.hpp>

namespace Zee
{

template <typename TVal, typename TIdx = uint32_t,
         class TIterator = storage_iterator_triplets<TVal, TIdx, false>>
class DSparseMatrixImage;

/** A (matrix) triplet is a tuplet (i, j, a_ij), representing an entry in a
  * matrix. It is particularly useful in the representation of sparse matrices.
  */
template <typename TVal, typename TIdx = int32_t>
class Triplet 
{
    public:
        Triplet(TIdx i, TIdx j, TIdx value)
        {
            _i = i;
            _j = j;
            _value = value;
        };

        ~Triplet() { }

        /** @return the value for this triplet */
        inline TVal value() const { return _value; }

        /** @return the row position inside the matrix */
        inline TIdx row() const { return _i; }

        /** @return the column position inside the matrix */
        inline TIdx col() const { return _j; }

    private:
        TIdx _i;
        TIdx _j;
        TVal _value;
};

//-----------------------------------------------------------------------------

/** Different partitioning schemes */
enum InitialPartitioning
{
    ONE_D_CYCLIC_PARTITIONING,
    ONE_D_BLOCK_PARTITIONING,
    RANDOM_PARTITIONING
};

/** The class DSparseMatrix is a distributed matrix type inspired by 
  * Eigen's SparseMatrix. It is distributed over multiple processing units,
  */
template <typename TVal, typename TIdx = uint32_t>
class DSparseMatrix : public DMatrixBase<TVal, TIdx>
{
    public:
        /** Initialize an (empty) sparse (rows x cols) oatrix */
        DSparseMatrix(TIdx rows, TIdx cols) :
            DMatrixBase<TVal, TIdx>(rows, cols)
        {
            _procs = 1;
            _partitioning = ONE_D_CYCLIC_PARTITIONING;
        }

        /** Default deconstructor */
        ~DSparseMatrix() { }

        /** Move constructor
          * FIXME: COPY RESOURCES
          */
        DSparseMatrix(DSparseMatrix&& o) = default;

        /** Sets the distribution scheme for this matrix */
        void setDistributionScheme(InitialPartitioning partitioning,
                TIdx procs)
        {
            _partitioning = partitioning;
            _procs = procs;
        }

        /** Multiply a sparse matrix with a dense vector */
        DVector<TVal, TIdx> operator*(const DVector<TVal, TIdx>& v) const
        {
            DVector<TVal, TIdx> u(this->rows());
            // how is u distributed, depends on left hand side.. very weird
            // construction, but interesting. Should return expression template
            // here
            // distributed shit
            return u;
        }

        /** @return the number of non-zero entries in the matrix */
        TIdx nonZeros() const
        {
            // TODO: implement
            return -1;
        }

        /** Construct a matrix from a set of triplets */
        template<typename TInputIterator>
        void setFromTriplets(
            const TInputIterator& begin,
            const TInputIterator& end)
        {
            if (!_subs.empty())
            {
                _subs.clear();
                _nz = 0;
            }

            for (TIdx i = 0; i < _procs; ++i)
                _subs.push_back(DSparseMatrixImage<TVal, TIdx>());

            for (TInputIterator it = begin; it != end; it++)
            {
                TIdx target_proc = 0;
                switch (_partitioning)
                {
                case ONE_D_CYCLIC_PARTITIONING:
                    target_proc = (*it).row() % _procs;
                    break;

                case ONE_D_BLOCK_PARTITIONING:
                    target_proc = (_procs * (*it).row()) / this->rows();
                    break;

                default:
                    // Fall back to 1D cyclic
                    target_proc = (*it).row() % _procs;
                    break;
                }

                _subs[target_proc].pushTriplet(*it);
                _nz++;
            }
        }

        // FIXME: delete
        void debugOutput()
        {
            int i = 0;
            for (auto& sub : _subs)
            {
                for(storage_iterator_triplets<TVal, TIdx, false> it = sub.begin();
                        it != sub.end(); ++it) {
                    cout << "(" << (*it).col() <<
                        ", " << (*it).row() <<
                        ", " << (*it).value() << ") " << i << endl;
                }
                ++i;
            }
        }

    private:
        TIdx _nz;

        // TODO: how do we reference other processors that store (part) of spm
        TIdx _procs;
        InitialPartitioning _partitioning;
        vector<DSparseMatrixImage<TVal, TIdx>> _subs;
};

// Owned by a processor. It is a submatrix, which holds the actual
// data, the 'global' DSparseMatrix can be seen as the sum of these images.
template <typename TVal, typename TIdx, class TIterator>
class DSparseMatrixImage
{
    public:
        /** Default constructor */
        DSparseMatrixImage() :
            _storage(new DSparseStorageTriplets<TVal, TIdx>())
        {
            // FIXME: Switch cases between different storage types
        }

        // Because we are using a unique pointer we need to move ownership
        // upon copying
        DSparseMatrixImage(DSparseMatrixImage&& other) :
            _storage(std::move(other._storage)) { }

        ~DSparseMatrixImage() { }

        void pushTriplet(Triplet<TVal, TIdx> t) {
            assert(_storage);
            _storage->pushTriplet(t);
        }

        typedef storage_iterator_triplets<TVal, TIdx, false> iterator;

        iterator begin()
        {
            return _storage->begin();
        }

        iterator end()
        {
            return _storage->end();
        }

    private:
        // Triplets, CRS or CCS
        unique_ptr<DSparseStorage<TVal, TIdx, TIterator>> _storage;
};

//-----------------------------------------------------------------------------
// Convenience functions (MATLAB syntax)
//-----------------------------------------------------------------------------

/** Create identity matrix as sparse matrix */
template <typename TIdx>
DSparseMatrix<double> eye(TIdx n)
{
    vector<Triplet<double, TIdx>> coefficients;
    coefficients.reserve(n);

    for (TIdx i = 0; i < n; ++i)
        coefficients.push_back(Triplet<double, TIdx>(i, i, 1.0));

    DSparseMatrix<double, TIdx> A(n, n);
    A.setFromTriplets(coefficients.begin(), coefficients.end());

    return A;
}

/** Create a random sparse (n x m) matrix */
template <typename TIdx>
DSparseMatrix<double, TIdx> rand(TIdx n, TIdx m, TIdx procs, double fill_in)
{
    vector<Triplet<double, TIdx>> coefficients;
    coefficients.reserve(n);

    for (TIdx i = 0; i < n; ++i)
        coefficients.push_back(Triplet<double, TIdx>(i, i, 1.0));

    DSparseMatrix<double, TIdx> A(n, m);
    A.setDistributionScheme(ONE_D_CYCLIC_PARTITIONING, procs);

    A.setFromTriplets(coefficients.begin(), coefficients.end());

    return A;
}

//-----------------------------------------------------------------------------

}
