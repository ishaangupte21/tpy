/*
    Frontend Driver
*/

use lexer::Lexer;
use parser::Parser;
use src_manager::SrcFileMetadata;
use std::io::Result;

pub fn run_compiler_frontend(src_file_path: &str) -> Result<()> {
    // First, we need to get the file metadata.
    let src_file_metadata = SrcFileMetadata::open_and_analyze_src_file(&src_file_path)?;

    // Now, we can create a lexer instance and a parser instance.
    // We must also initialize a vector for the AST.
    let mut abstract_syntax_tree = Vec::new();

    let lexer = Lexer::new(&src_file_metadata);
    let mut parser = Parser::new(lexer, &mut abstract_syntax_tree);

    let ast_root_node = match parser.parse_py_compilation_unit() {
        Ok(index) => index,
        Err(_) => 0,
    };

    println!("{:?}", abstract_syntax_tree);

    Ok(())
}
