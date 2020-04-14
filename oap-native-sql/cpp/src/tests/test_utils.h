#include <arrow/array.h>
#include <arrow/buffer.h>
#include <arrow/ipc/json_simple.h>
#include <arrow/memory_pool.h>
#include <arrow/pretty_print.h>
#include <arrow/record_batch.h>
#include <arrow/status.h>
#include <arrow/type.h>
#include <gandiva/node.h>
#include <gandiva/tree_expr_builder.h>
#include <iostream>
#include <memory>
#include <sstream>
#include "utils/macros.h"
using namespace arrow;

using TreeExprBuilder = gandiva::TreeExprBuilder;
using FunctionNode = gandiva::FunctionNode;

#define ASSERT_NOT_OK(status)                  \
  do {                                         \
    ::arrow::Status __s = (status);            \
    if (!__s.ok()) {                           \
      throw std::runtime_error(__s.message()); \
    }                                          \
  } while (false);

#define ARROW_ASSIGN_OR_THROW_IMPL(status_name, lhs, rexpr) \
  do {                                                      \
    auto status_name = (rexpr);                             \
    auto __s = status_name.status();                        \
    if (!__s.ok()) {                                        \
      throw std::runtime_error(__s.message());              \
    }                                                       \
    lhs = std::move(status_name).ValueOrDie();              \
  } while (false);

#define ARROW_ASSIGN_OR_THROW_NAME(x, y) ARROW_CONCAT(x, y)

#define ARROW_ASSIGN_OR_THROW(lhs, rexpr)                                              \
  ARROW_ASSIGN_OR_THROW_IMPL(ARROW_ASSIGN_OR_THROW_NAME(_error_or_value, __COUNTER__), \
                             lhs, rexpr);

template <typename T>
Status Equals(const T& expected, const T& actual) {
  if (expected.Equals(actual)) {
    return arrow::Status::OK();
  }
  std::stringstream pp_expected;
  std::stringstream pp_actual;
  ::arrow::PrettyPrintOptions options(/*indent=*/2);
  options.window = 50;
  ASSERT_NOT_OK(PrettyPrint(expected, options, &pp_expected));
  ASSERT_NOT_OK(PrettyPrint(actual, options, &pp_actual));
  if (pp_expected.str() == pp_actual.str()) {
    return arrow::Status::OK();
  }
  return Status::Invalid("Expected RecordBatch is ", pp_expected.str(), " with schema ",
                         expected.schema()->ToString(), ", while actual is ",
                         pp_actual.str(), " with schema ", actual.schema()->ToString());
}

void MakeInputBatch(std::vector<std::string> input_data,
                    std::shared_ptr<arrow::Schema> sch,
                    std::shared_ptr<arrow::RecordBatch>* input_batch) {
  // prepare input record Batch
  std::vector<std::shared_ptr<Array>> array_list;
  int length = -1;
  int i = 0;
  for (auto data : input_data) {
    std::shared_ptr<Array> a0;
    ASSERT_NOT_OK(arrow::ipc::internal::json::ArrayFromJSON(sch->field(i++)->type(),
                                                            data.c_str(), &a0));
    if (length == -1) {
      length = a0->length();
    }
    assert(length == a0->length());
    array_list.push_back(a0);
  }

  *input_batch = RecordBatch::Make(sch, length, array_list);
  return;
}
