#include "mbasic/ast.hpp"

namespace mbasic {

// Deep clone an expression
Expr clone_expr(const Expr& e) {
    return std::visit([](const auto& ptr) -> Expr {
        using T = std::decay_t<decltype(*ptr)>;

        if constexpr (std::is_same_v<T, NumberExpr>) {
            return make_expr<NumberExpr>(ptr->value, ptr->line, ptr->column);
        }
        else if constexpr (std::is_same_v<T, StringExpr>) {
            return make_expr<StringExpr>(ptr->value, ptr->line, ptr->column);
        }
        else if constexpr (std::is_same_v<T, VariableExpr>) {
            return make_expr<VariableExpr>(ptr->name, ptr->original, ptr->type, ptr->line, ptr->column);
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return make_expr<BinaryExpr>(
                ptr->op,
                clone_expr(ptr->left),
                clone_expr(ptr->right),
                ptr->line, ptr->column
            );
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return make_expr<UnaryExpr>(
                ptr->op,
                clone_expr(ptr->operand),
                ptr->line, ptr->column
            );
        }
        else if constexpr (std::is_same_v<T, FunctionCallExpr>) {
            std::vector<Expr> args;
            for (const auto& arg : ptr->args) {
                args.push_back(clone_expr(arg));
            }
            return make_expr<FunctionCallExpr>(ptr->name, std::move(args), ptr->line, ptr->column);
        }
        else if constexpr (std::is_same_v<T, ArrayAccessExpr>) {
            std::vector<Expr> indices;
            for (const auto& idx : ptr->indices) {
                indices.push_back(clone_expr(idx));
            }
            return make_expr<ArrayAccessExpr>(
                ptr->name, ptr->original, std::move(indices), ptr->type, ptr->line, ptr->column
            );
        }
        else {
            // Should never reach here
            return make_expr<NumberExpr>(0, 0, 0);
        }
    }, e);
}

} // namespace mbasic
