/*
 * Copyright (c) 2011-2013  University of Texas at Austin. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This file is part of PerfExpert.
 *
 * PerfExpert is free software: you can redistribute it and/or modify it under
 * the terms of the The University of Texas at Austin Research License
 * 
 * PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 * 
 * Authors: Leonardo Fialho and Ashay Rane
 *
 * $HEADER$
 */

#ifndef TOOLS_MACPO_INCLUDE_BRANCHPATH_H_
#define TOOLS_MACPO_INCLUDE_BRANCHPATH_H_

#include <VariableRenaming.h>

#include <map>
#include <string>
#include <vector>

#include "analysis_profile.h"
#include "generic_defs.h"
#include "inst_defs.h"
#include "loop_traversal.h"

class branchpath_t {
 public:
    typedef std::map<SgExpression*, loop_info_t*> expr_map_t;
    typedef std::vector<SgPntrArrRefExp*> pntr_list_t;
    typedef std::vector<SgNode*> node_list_t;
    typedef std::map<std::string, node_list_t> sstore_map_t;

    explicit branchpath_t(VariableRenaming*& _var_renaming);
    ~branchpath_t();

    void atTraversalEnd();
    void atTraversalStart();

    void process_node(SgNode* node);
    void process_loop(SgScopeStatement* scope_stmt,
            loop_info_t& loop_info_t, expr_map_t& loop_map,
            name_list_t& stream_list);

    const analysis_profile_t& get_analysis_profile();

    const statement_list_t::iterator stmt_begin();
    const statement_list_t::iterator stmt_end();

 private:
    void instrument_branches(Sg_File_Info* fileInfo,
            SgScopeStatement* scope_stmt, SgExpression* idxv_expr);

    VariableRenaming* var_renaming;
    statement_list_t statement_list;
    analysis_profile_t analysis_profile;
    loop_traversal_t* loop_traversal;
};

#endif  // TOOLS_MACPO_INCLUDE_BRANCHPATH_H_