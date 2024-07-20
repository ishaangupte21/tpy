/*
    Frontend Driver
*/

use lexer::{token::TokenKind, Lexer};
use src_manager::SrcFileMetadata;
use std::io::Result;

pub fn run_compiler_frontend(src_file_path: &str) -> Result<()> {
    // First, we need to get the file metadata.
    let src_file_metadata = SrcFileMetadata::open_and_analyze_src_file(&src_file_path)?;

    // Now, we can create a lexer instance.
    let mut lexer = Lexer::new(&src_file_metadata);

    let mut tok = lexer.next();
    while tok.kind != TokenKind::End {
        println!("{:?}", tok);
        tok = lexer.next();
    }

    Ok(())
}
