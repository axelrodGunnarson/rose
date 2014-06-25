/** 
 * \file MDCG/include/MDCG/code-generator.hpp
 *
 * \author Tristan Vanderbruggen
 *
 */

#ifndef __MDCG_CODE_GENERATOR_HPP__
#define __MDCG_CODE_GENERATOR_HPP__

#include "MDCG/model.hpp"
#include "MFB/Sage/class-declaration.hpp"

#include "MFB/Sage/driver.hpp"

#include <string>
#include <sstream>
#include <iterator>

#include "sage3basic.h"

namespace MDCG {

/*!
 * \addtogroup grp_mdcg_codegen
 * @{
*/

class CodeGenerator {
  private:
    static unsigned s_var_gen_cnt;
  
    MFB::Driver<MFB::Sage> & p_mfb_driver;

    SgVariableSymbol * instantiateDeclaration(std::string decl_name, unsigned file_id, SgType * type, SgInitializer * init) const;

  public:
    CodeGenerator(MFB::Driver<MFB::Sage> & mfb_driver);

    MFB::Driver<MFB::Sage> & getDriver() const;

    template <class ModelTraversal>
    SgInitializer * createInitializer(
      Model::class_t element,
      const typename ModelTraversal::input_t & input,
      unsigned file_id
    ) const {
      SgExprListExp * expr_list = SageBuilder::buildExprListExp();

      p_mfb_driver.useSymbol<SgClassDeclaration>(element->node->symbol, file_id);

      std::vector<Model::field_t>::const_iterator it_field = element->scope->field_children.begin();
      SgExpression * expr = ModelTraversal::createFieldInitializer(*this, *it_field, 0, input, file_id);
      if (expr == NULL) return NULL;
      it_field++;
      unsigned field_id = 1;
      for (; it_field != element->scope->field_children.end(); it_field++) {
        expr = ModelTraversal::createFieldInitializer(*this, *it_field, field_id++, input, file_id);
        assert(expr != NULL);
        expr_list->append_expression(expr);
      }

      return SageBuilder::buildAggregateInitializer(expr_list);
    }

    template <class ModelTraversal>
    SgExpression * createPointer(
      Model::class_t element,
      const typename ModelTraversal::input_t & input,
      unsigned file_id,
      const std::string & decl_name
    ) const {
      SgInitializer * initializer = createInitializer<ModelTraversal>(element, input, file_id);
      assert(initializer != NULL);

      p_mfb_driver.useSymbol<SgClassDeclaration>(element->node->symbol, file_id, false);

      SgType * type = element->node->symbol->get_type();
      assert(type != NULL);

      SgVariableSymbol * symbol = instantiateDeclaration(decl_name, file_id, type, initializer);
      assert(symbol != NULL);

      return SageBuilder::buildAddressOfOp(SageBuilder::buildVarRefExp(symbol));
    }

    template <class ModelTraversal, class Iterator>
    SgAggregateInitializer * createArray(
      Model::class_t element,
      Iterator input_begin,
      Iterator input_end,
      unsigned file_id
    ) const {
      SgExprListExp * expr_list = SageBuilder::buildExprListExp();

      Iterator it;
      for (it = input_begin; it != input_end; it++) {
        SgExpression * expr = createInitializer<ModelTraversal>(element, *it, file_id);
        if (expr != NULL)
          expr_list->append_expression(expr);
      }

      return SageBuilder::buildAggregateInitializer(expr_list);
    }

    template <class ModelTraversal, class Iterator>
    SgAggregateInitializer * createPointerArray(
      Model::class_t element,
      Iterator input_begin,
      Iterator input_end,
      unsigned file_id,
      const std::string & decl_prefix
    ) const {
      SgExprListExp * expr_list = SageBuilder::buildExprListExp();

      unsigned cnt = 0;

      Iterator it;
      for (it = input_begin; it != input_end; it++) {
        SgExpression * expr = createPointer<ModelTraversal>(element, *it, file_id, decl_name.str())
        if (expr != NULL) {
          std::ostringstream decl_name;
          decl_name << decl_prefix << "_" << cnt++;
          expr_list->append_expression(expr);
        }
      }

      return SageBuilder::buildAggregateInitializer(expr_list);
    }

    template <class ModelTraversal, class Iterator>
    SgExpression * createArrayPointer(
      Model::class_t element,
      unsigned num_element,
      Iterator input_begin,
      Iterator input_end,
      unsigned file_id,
      const std::string & decl_name
    ) const {
      SgAggregateInitializer * aggr_init = createArray<ModelTraversal, Iterator>(element, input_begin, input_end, file_id);

      p_mfb_driver.useSymbol<SgClassDeclaration>(element->node->symbol, file_id);

      SgType * type = element->node->symbol->get_type();
      assert(type != NULL);
      type = SageBuilder::buildArrayType(type, SageBuilder::buildUnsignedLongVal(num_element));
      assert(type != NULL);

      SgVariableSymbol * symbol = instantiateDeclaration(decl_name, file_id, type, aggr_init);
      assert(symbol != NULL);

      return SageBuilder::buildVarRefExp(symbol);
    }

    template <class ModelTraversal, class Iterator>
    SgExpression * createPointerArrayPointer(
      Model::class_t element,
      unsigned num_element,
      Iterator input_begin,
      Iterator input_end,
      unsigned file_id,
      const std::string & decl_name,
      const std::string & sub_decl_prefix
    ) const {
      SgAggregateInitializer * aggr_init = createPointerArray<ModelTraversal, Iterator>(
                                             element,
                                             input_begin,
                                             input_end,
                                             file_id,
                                             sub_decl_prefix
                                           );

      p_mfb_driver.useSymbol<SgClassDeclaration>(element->node->symbol, file_id);

      SgType * type = element->node->symbol->get_type();
      assert(type != NULL);
      type = SageBuilder::buildPointerType(type);
      assert(type != NULL);
      type = SageBuilder::buildArrayType(type, SageBuilder::buildUnsignedLongVal(num_element));
      assert(type != NULL);

      SgVariableSymbol * symbol = instantiateDeclaration(decl_name, file_id, type, aggr_init);
      assert(symbol != NULL);

      return SageBuilder::buildVarRefExp(symbol);
    }

    template <class ModelTraversal>
    SgVariableSymbol * addDeclaration(
      Model::class_t element,
      typename ModelTraversal::input_t & input,
      unsigned file_id,
      std::string decl_name
    ) const {
      SgInitializer * initializer = createInitializer<ModelTraversal>(element, input, file_id);
      assert(initializer != NULL);

      SgType * type = element->node->symbol->get_type();
      assert(type != NULL);

      SgVariableSymbol * symbol = instantiateDeclaration(decl_name, file_id, type, initializer);
      assert(symbol != NULL);
      
      return symbol;
    }
};

/** @} */

}

#endif /* __MDCG_CODE_GENERATOR_HPP__ */

