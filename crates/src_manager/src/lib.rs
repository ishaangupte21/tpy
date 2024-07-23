/*
    Source file manager for this interpreter.
*/

use std::{collections::HashMap, fmt::Debug, fs, io::Result};

use span::Span;

pub mod span;

// This structure will store file metadata.
#[derive(Debug)]
pub struct SrcFileMetadata<'a> {
    pub len: usize,
    pub multi_byte_chars: HashMap<usize, usize>,
    pub newline_chars: Vec<NewLineChar>,
    pub contents: String,
    pub path: &'a str,
}

#[derive(Debug)]
pub struct NewLineChar {
    pub pos: usize,
    pub len: i32,
}

impl NewLineChar {
    pub fn new(pos: usize, len: i32) -> Self {
        Self { pos, len }
    }
}

impl SrcFileMetadata<'_> {
    pub fn open_and_analyze_src_file<'a>(path: &'a str) -> Result<SrcFileMetadata<'a>> {
        let contents = fs::read_to_string(path)?;

        let mut multi_byte_chars: HashMap<usize, usize> = HashMap::new();
        let mut newline_chars: Vec<NewLineChar> = Vec::new();

        Self::analyze_src_file(&contents, &mut multi_byte_chars, &mut newline_chars);

        Ok(SrcFileMetadata {
            len: contents.len(),
            multi_byte_chars,
            newline_chars,
            contents,
            path,
        })
    }

    fn analyze_src_file(
        contents: &str,
        multi_byte_chars: &mut HashMap<usize, usize>,
        newline_chars: &mut Vec<NewLineChar>,
    ) {
        // Iterate through the source file, looking for multi byte characters and newline characters.
        let mut chars = contents.chars();
        let mut pos: usize = 0;

        while let Some(c) = chars.next() {
            match c {
                '\n' => {
                    newline_chars.push(NewLineChar::new(pos, 1));
                    pos += 1;
                }
                '\r' => {
                    // Check the next character for \r\n
                    if chars.clone().next().unwrap_or('\0') == '\n' {
                        newline_chars.push(NewLineChar::new(pos, 2));
                        pos += 2;
                        chars.next();
                    } else {
                        newline_chars.push(NewLineChar::new(pos, 1));
                        pos += 1;
                    }
                }

                c if c > '\x7f' => {
                    multi_byte_chars.insert(pos, c.len_utf8());
                    pos += 1
                }

                _ => pos += 1,
            }
        }
    }

    pub fn get_src_file_text<'a>(&'a self, span: &Span) -> &'a str {
        &self.contents[span.start..span.end]
    }

    pub fn get_src_file_text_checked<'a>(&'a self, span: &Span) -> &'a str {
        if span.end < self.contents.len() {
            &self.contents[span.start..span.end]
        } else {
            &self.contents[span.start..]
        }
    }
}
