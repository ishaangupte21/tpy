/*
    Parser context for verifying proper semantics.
*/

use src_manager::span::Span;

use crate::Parser;

pub(crate) type ContextStack = Vec<ParserContext>;

pub(crate) enum ParserContext {
    ForLoop,
    WhileLoop,
    FunctionDecl,
    ClassDecl,
}

/*
    We will implement context verification here.
*/
impl Parser<'_> {
    pub(crate) fn check_break_stmt_in_valid_context(&mut self, span: Span) {
        let mut context_stack_iter = self.context_stack.iter().rev();

        while let Some(ctx) = context_stack_iter.next() {
            match ctx {
                ParserContext::ForLoop | ParserContext::WhileLoop => return,
                ParserContext::ClassDecl => {
                    // If we encounter a class declaration before a loop, we need to see if the class is within a loop.
                    while let Some(next_ctx) = context_stack_iter.next() {
                        if matches!(next_ctx, ParserContext::ForLoop | ParserContext::WhileLoop) {
                            self.report_parse_error("cannot have 'break' statement in nested class definition within for loop.", span);
                            return;
                        }
                    }

                    // If a loop is never found, break to report the generic error message.
                    break;
                }
                ParserContext::FunctionDecl => {
                    // If we encounter a function declaration before a loop, we need to see if the function is within a loop.
                    while let Some(next_ctx) = context_stack_iter.next() {
                        if matches!(next_ctx, ParserContext::ForLoop | ParserContext::WhileLoop) {
                            self.report_parse_error("cannot have 'break' statement in nested function definition within for loop.", span);
                            return;
                        }
                    }

                    // If a loop is never found, break to report the generic error message.
                    break;
                }
            }
        }

        // Here, we will report a generic error message when no loop is found.
        self.report_parse_error("'break' statements must be within a loop.", span);
    }

    pub(crate) fn check_continue_stmt_in_valid_context(&mut self, span: Span) {
        let mut context_stack_iter = self.context_stack.iter().rev();

        while let Some(ctx) = context_stack_iter.next() {
            match ctx {
                ParserContext::ForLoop | ParserContext::WhileLoop => return,
                ParserContext::ClassDecl => {
                    // If we encounter a class declaration before a loop, we need to see if the class is within a loop.
                    while let Some(next_ctx) = context_stack_iter.next() {
                        if matches!(next_ctx, ParserContext::ForLoop | ParserContext::WhileLoop) {
                            self.report_parse_error("cannot have 'continue' statement in nested class definition within for loop.", span);
                            return;
                        }
                    }

                    // If a loop is never found, break to report the generic error message.
                    break;
                }
                ParserContext::FunctionDecl => {
                    // If we encounter a function declaration before a loop, we need to see if the function is within a loop.
                    while let Some(next_ctx) = context_stack_iter.next() {
                        if matches!(next_ctx, ParserContext::ForLoop | ParserContext::WhileLoop) {
                            self.report_parse_error("cannot have 'continue' statement in nested function definition within for loop.", span);
                            return;
                        }
                    }

                    // If a loop is never found, break to report the generic error message.
                    break;
                }
            }
        }

        // Here, we will report a generic error message when no loop is found.
        self.report_parse_error("'continue' statements must be within a loop.", span);
    }
}
