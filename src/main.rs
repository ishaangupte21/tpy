use std::{env::args, process::exit};

use compiler_driver::run_compiler;

// Entry point
fn main() {
    // First, we need to get the source file path from CLI args.
    let src_file_path = match args().nth(1) {
        Some(arg) => arg,
        None => {
            eprintln!("error: no source files given.");
            exit(1);
        }
    };

    // Take the source file path and run the compiler frontend.
    // Wrapping it in a function allows the source file contents and AST to be dropped once they are no longer needed.
    match run_compiler(&src_file_path) {
        Ok(_) => {}
        Err(e) => {
            eprintln!("error: {e}");
            exit(1);
        }
    };
}
