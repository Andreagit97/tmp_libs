// SPDX-License-Identifier: Apache-2.0
/*
Copyright (C) 2023 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include <libsinsp/filter/ast.h>
#include <libsinsp/filter/escaping.h>

using namespace libsinsp::filter::ast;

void base_expr_visitor::visit(and_expr* e) {
	for(auto& c : e->children) {
		if(m_should_stop_visit) {
			return;
		}
		c->accept(this);
	}
}

void base_expr_visitor::visit(or_expr* e) {
	for(auto& c : e->children) {
		if(m_should_stop_visit) {
			return;
		}
		c->accept(this);
	}
}

void base_expr_visitor::visit(not_expr* e) {
	if(!m_should_stop_visit) {
		e->child->accept(this);
	}
}

void base_expr_visitor::visit(binary_check_expr* e) {
	if(!m_should_stop_visit) {
		e->left->accept(this);
	}

	if(!m_should_stop_visit) {
		e->right->accept(this);
	}
}

void base_expr_visitor::visit(unary_check_expr* e) {
	if(!m_should_stop_visit) {
		e->left->accept(this);
	}
}

void base_expr_visitor::visit(field_transformer_expr* e) {
	if(!m_should_stop_visit) {
		e->value->accept(this);
	}
}

void base_expr_visitor::visit(identifier_expr* e) {}

void base_expr_visitor::visit(value_expr* e) {}

void base_expr_visitor::visit(list_expr* e) {}

void base_expr_visitor::visit(field_expr* e) {}

void const_base_expr_visitor::visit(const and_expr* e) {
	for(auto& c : e->children) {
		if(m_should_stop_visit) {
			return;
		}
		c->accept(this);
	}
}

void const_base_expr_visitor::visit(const or_expr* e) {
	for(auto& c : e->children) {
		if(m_should_stop_visit) {
			return;
		}
		c->accept(this);
	}
}

void const_base_expr_visitor::visit(const not_expr* e) {
	if(!m_should_stop_visit) {
		e->child->accept(this);
	}
}

void const_base_expr_visitor::visit(const binary_check_expr* e) {
	if(!m_should_stop_visit) {
		e->left->accept(this);
	}
	if(!m_should_stop_visit) {
		e->right->accept(this);
	}
}

void const_base_expr_visitor::visit(const unary_check_expr* e) {
	if(!m_should_stop_visit) {
		e->left->accept(this);
	}
}

void const_base_expr_visitor::visit(const field_transformer_expr* e) {
	if(!m_should_stop_visit) {
		e->value->accept(this);
	}
}

void const_base_expr_visitor::visit(const identifier_expr* e) {}

void const_base_expr_visitor::visit(const value_expr* e) {}

void const_base_expr_visitor::visit(const list_expr* e) {}

void const_base_expr_visitor::visit(const field_expr* e) {}

void string_visitor::visit_logical_op(const char* op,
                                      const std::vector<std::unique_ptr<expr>>& children) {
	bool first = true;

	m_str += "(";
	for(auto& c : children) {
		if(!first) {
			m_str += " ";
			m_str += op;
			m_str += " ";
		}
		first = false;
		c->accept(this);
	}
	m_str += ")";
}

void string_visitor::visit(const and_expr* e) {
	visit_logical_op("and", e->children);
}

void string_visitor::visit(const or_expr* e) {
	visit_logical_op("or", e->children);
}

void string_visitor::visit(const not_expr* e) {
	m_str += "not ";
	e->child->accept(this);
}

void string_visitor::visit(const identifier_expr* e) {
	m_str += e->identifier;
}

void string_visitor::visit(const value_expr* e) {
	m_str += libsinsp::filter::escape_str(e->value);
}

void string_visitor::visit(const list_expr* e) {
	bool first = true;

	m_str += "(";
	for(auto& val : e->values) {
		if(!first) {
			m_str += ", ";
		}
		first = false;
		m_str += libsinsp::filter::escape_str(val);
	}
	m_str += ")";
}
void string_visitor::visit(const unary_check_expr* e) {
	e->left->accept(this);
	m_str += " ";
	m_str += e->op;
}

void string_visitor::visit(const binary_check_expr* e) {
	e->left->accept(this);
	m_str += " ";
	m_str += e->op;
	m_str += " ";
	e->right->accept(this);
}

void string_visitor::visit(const field_expr* e) {
	m_str += e->field;
	if(e->arg != "") {
		m_str += "[" + libsinsp::filter::escape_str(e->arg) + "]";
	}
}

void string_visitor::visit(const field_transformer_expr* e) {
	m_str += e->transformer;
	m_str += "(";
	e->value->accept(this);
	m_str += ")";
}

const std::string& string_visitor::as_string() {
	return m_str;
}

std::string libsinsp::filter::ast::as_string(const ast::expr* e) {
	string_visitor sv;
	e->accept(&sv);
	return sv.as_string();
}

std::unique_ptr<expr> libsinsp::filter::ast::clone(const expr* e) {
	struct clone_visitor : public const_expr_visitor {
		std::unique_ptr<expr> m_last_node;

		void visit(const and_expr* e) override {
			std::vector<std::unique_ptr<expr>> children;
			for(auto& c : e->children) {
				c->accept(this);
				children.push_back(std::move(m_last_node));
			}
			m_last_node = and_expr::create(children, e->get_pos());
		}

		void visit(const or_expr* e) override {
			std::vector<std::unique_ptr<expr>> children;
			for(auto& c : e->children) {
				c->accept(this);
				children.push_back(std::move(m_last_node));
			}
			m_last_node = or_expr::create(children, e->get_pos());
		}

		void visit(const not_expr* e) override {
			e->child->accept(this);
			m_last_node = not_expr::create(std::move(m_last_node), e->get_pos());
		}

		void visit(const binary_check_expr* e) override {
			e->left->accept(this);
			auto left = std::move(m_last_node);
			e->right->accept(this);
			auto right = std::move(m_last_node);
			m_last_node = binary_check_expr::create(std::move(left),
			                                        e->op,
			                                        std::move(right),
			                                        e->get_pos());
		}

		void visit(const unary_check_expr* e) override {
			e->left->accept(this);
			auto left = std::move(m_last_node);
			m_last_node = unary_check_expr::create(std::move(left), e->op, e->get_pos());
		}

		void visit(const identifier_expr* e) override {
			m_last_node = identifier_expr::create(e->identifier, e->get_pos());
		}

		void visit(const value_expr* e) override {
			m_last_node = value_expr::create(e->value, e->get_pos());
		}

		void visit(const list_expr* e) override {
			m_last_node = list_expr::create(e->values, e->get_pos());
		}

		void visit(const field_expr* e) override {
			m_last_node = field_expr::create(e->field, e->arg, e->get_pos());
		}

		void visit(const field_transformer_expr* e) override {
			e->value->accept(this);
			auto value = std::move(m_last_node);
			m_last_node =
			        field_transformer_expr::create(e->transformer, std::move(value), e->get_pos());
		}
	} visitor;

	e->accept(&visitor);
	return std::move(visitor.m_last_node);
}
