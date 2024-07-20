use std::{env::args, process::exit};

use frontend_driver::run_compiler_frontend;

// Entry point
fn main() {
    // First, we need to get the source file path from CLI args.
    let src_file_path = match args().skip(1).next() {
        Some(arg) => arg,
        None => {
            eprintln!("error: no source files given.");
            exit(1);
        }
    };

    // Take the source file path and run the compiler frontend.
    // Wrapping it in a function allows the source file contents and AST to be dropped once they are no longer needed.
    match run_compiler_frontend(&src_file_path) {
        Ok(_) => {}
        Err(e) => {
            eprintln!("error: {e}");
            exit(1);
        }
    };
}
