/*
 Frontend error manager
*/

use std::io::{stderr, BufWriter, Write};

use src_manager::{span::Span, SrcFileMetadata};
use unicode_width::UnicodeWidthStr;

pub fn report_frontend_error(msg: &str, loc: Span, metadata: &SrcFileMetadata) {
    let (line, col, line_start, line_end) = compute_line_and_col(&loc, metadata);

    // We will use a buffered writer here to avoid an exuberant number of system calls.
    // For now, we will ignore errors that come with writing to the buffered writer.
    let mut buffer = BufWriter::new(stderr());

    // First, print the error message to the user.
    let _ = writeln!(
        buffer,
        "error: {}\n --> {} at line {}, column {}",
        msg, &metadata.path, line, col
    );

    // Now, we must get the line text and print it.
    let line_text = metadata.get_src_file_text(&Span::new(line_start, line_end));
    let _ = writeln!(buffer, " {line_text}");

    // Compute the number of spaces to the left of the offending text.
    let prior_contents_width = metadata
        .get_src_file_text(&Span::new(line_start, loc.start))
        .width();

    for _ in 0..=prior_contents_width {
        let _ = buffer.write(b" ");
    }

    // Now, compute the width of the offending text.
    let offending_text_render_width = metadata.get_src_file_text(&loc).width();
    for _ in 0..offending_text_render_width {
        let _ = buffer.write(b"^");
    }

    let _ = buffer.write(b"\n");

    let _ = buffer.flush();
}

/*
    This function computes the line and column numbers for a given location in source code.
    It also returns the line starting and ending positions.
*/
fn compute_line_and_col(loc: &Span, metadata: &SrcFileMetadata) -> (usize, usize, usize, usize) {
    // First, we need to compute the line number.
    // For one line programs, we know the line is 1.
    let line = if metadata.newline_chars.is_empty() {
        1
    } else {
        let mut i = 0;
        for newline_char in &metadata.newline_chars {
            // We need to linearly search the vector of newline characters until we find the first one with an index greater than the starting position.
            if newline_char.pos > loc.start {
                break;
            } else {
                i += 1;
            }
        }

        i + 1
    };

    // Now, we need to compute the column number.
    let line_start = if line == 1 {
        0 as usize
    } else {
        let newline_char = &metadata.newline_chars[line - 2];
        newline_char.pos + newline_char.len as usize
    };

    // Now, we need to compute the column number.
    let mut col = 1;
    let mut pos = line_start;
    while pos < loc.start {
        col += 1;
        pos += match metadata.multi_byte_chars.get(&pos) {
            Some(v) => *v,
            None => 1,
        };
    }

    // Compute the ending position of the line
    // if we are on the last line, the end is the end of the file.
    // Otherwise, we need to go one back from the current line number to adjust for zero indexing.
    let line_end = if line == metadata.newline_chars.len() + 1 {
        metadata.contents.len()
    } else {
        metadata.newline_chars[line - 1].pos
    };

    (line, col, line_start, line_end)
}
