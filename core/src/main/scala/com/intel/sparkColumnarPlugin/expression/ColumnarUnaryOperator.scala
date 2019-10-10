package com.intel.sparkColumnarPlugin.expression

import com.google.common.collect.Lists

import org.apache.arrow.gandiva.evaluator._
import org.apache.arrow.gandiva.exceptions.GandivaException
import org.apache.arrow.gandiva.expression._
import org.apache.arrow.vector.types.pojo.ArrowType
import org.apache.arrow.vector.types.pojo.Field

import org.apache.spark.internal.Logging
import org.apache.spark.sql.catalyst.expressions._
import org.apache.spark.sql.types._

import scala.collection.mutable.ListBuffer
/**
 * A version of add that supports columnar processing for longs.
 */
class ColumnarIsNotNull(child: Expression, original: Expression)
  extends IsNotNull(child: Expression) with ColumnarExpression with Logging {
  override def doColumnarCodeGen(args: java.lang.Object): (TreeNode, ArrowType) = {
    val (child_node, childType): (TreeNode, ArrowType) = child.asInstanceOf[ColumnarExpression].doColumnarCodeGen(args)

    val resultType = new ArrowType.Bool()
    val funcNode = TreeBuilder.makeFunction(
      "isnotnull", Lists.newArrayList(child_node), resultType)
    (funcNode, resultType)
  }
}

object ColumnarUnaryOperator {

  def create(child: Expression, original: Expression): Expression = original match {
    case i: IsNotNull =>
      new ColumnarIsNotNull(child, i)
    case c: Cast =>
      child
    case other =>
      throw new UnsupportedOperationException(s"not currently supported: $other.")
  }
}
