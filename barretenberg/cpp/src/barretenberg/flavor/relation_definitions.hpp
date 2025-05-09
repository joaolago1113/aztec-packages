// === AUDIT STATUS ===
// internal:    { status: not started, auditors: [], date: YYYY-MM-DD }
// external_1:  { status: not started, auditors: [], date: YYYY-MM-DD }
// external_2:  { status: not started, auditors: [], date: YYYY-MM-DD }
// =====================

#pragma once

#include "barretenberg/relations/relation_types.hpp"

// Utilities for explicit instantiation of relation and permutation classes.

#define ExtendedEdge(Flavor) Flavor::ExtendedEdges
#define EvaluationEdge(Flavor) Flavor::AllValues
#define EntityEdge(Flavor) Flavor::AllEntities<Flavor::FF>

#define ACCUMULATE(...) _ACCUMULATE(__VA_ARGS__)
#define _ACCUMULATE(RelationImpl, Flavor, AccumulatorType, EdgeType)                                                   \
    template void                                                                                                      \
    RelationImpl<Flavor::FF>::accumulate<bb::Relation<RelationImpl<Flavor::FF>>::AccumulatorType, EdgeType(Flavor)>(   \
        bb::Relation<RelationImpl<Flavor::FF>>::AccumulatorType&,                                                      \
        EdgeType(Flavor) const&,                                                                                       \
        RelationParameters<Flavor::FF> const&,                                                                         \
        Flavor::FF const&);

#define PERMUTATION_METHOD(...) _PERMUTATION_METHOD(__VA_ARGS__)
#define _PERMUTATION_METHOD(MethodName, RelationImpl, Flavor, AccumulatorType, EdgeType)                               \
    template typename bb::Relation<RelationImpl<Flavor::FF>>::AccumulatorType                                          \
    RelationImpl<Flavor::FF>::MethodName<bb::Relation<RelationImpl<Flavor::FF>>::AccumulatorType, EdgeType(Flavor)>(   \
        EdgeType(Flavor) const&, RelationParameters<Flavor::FF> const&);

#define DEFINE_SUMCHECK_RELATION_CLASS(RelationImpl, Flavor)                                                           \
    ACCUMULATE(RelationImpl, Flavor, SumcheckTupleOfUnivariatesOverSubrelations, ExtendedEdge)                         \
    ACCUMULATE(RelationImpl, Flavor, SumcheckArrayOfValuesOverSubrelations, EvaluationEdge)                            \
    ACCUMULATE(RelationImpl, Flavor, SumcheckArrayOfValuesOverSubrelations, EntityEdge)

#define DEFINE_ZK_SUMCHECK_RELATION_CLASS(RelationImpl, Flavor)                                                        \
    ACCUMULATE(RelationImpl, Flavor, ZKSumcheckTupleOfUnivariatesOverSubrelations, ExtendedEdge)                       \
    ACCUMULATE(RelationImpl, Flavor, SumcheckArrayOfValuesOverSubrelations, EvaluationEdge)                            \
    ACCUMULATE(RelationImpl, Flavor, SumcheckArrayOfValuesOverSubrelations, EntityEdge)

#define DEFINE_SUMCHECK_VERIFIER_RELATION_CLASS(RelationImpl, Flavor)                                                  \
    ACCUMULATE(RelationImpl, Flavor, SumcheckArrayOfValuesOverSubrelations, EvaluationEdge)

#define DEFINE_SUMCHECK_PERMUTATION_CLASS(RelationImpl, Flavor)                                                        \
    PERMUTATION_METHOD(compute_grand_product_numerator, RelationImpl, Flavor, UnivariateAccumulator0, ExtendedEdge)    \
    PERMUTATION_METHOD(compute_grand_product_numerator, RelationImpl, Flavor, ValueAccumulator0, EvaluationEdge)       \
    PERMUTATION_METHOD(compute_grand_product_numerator, RelationImpl, Flavor, ValueAccumulator0, EntityEdge)           \
    PERMUTATION_METHOD(compute_grand_product_denominator, RelationImpl, Flavor, UnivariateAccumulator0, ExtendedEdge)  \
    PERMUTATION_METHOD(compute_grand_product_denominator, RelationImpl, Flavor, ValueAccumulator0, EvaluationEdge)     \
    PERMUTATION_METHOD(compute_grand_product_denominator, RelationImpl, Flavor, ValueAccumulator0, EntityEdge)

#define DEFINE_SUMCHECK_VERIFIER_PERMUTATION_CLASS(RelationImpl, Flavor)                                               \
    PERMUTATION_METHOD(compute_grand_product_numerator, RelationImpl, Flavor, ValueAccumulator0, EvaluationEdge)       \
    PERMUTATION_METHOD(compute_grand_product_denominator, RelationImpl, Flavor, ValueAccumulator0, EvaluationEdge)
