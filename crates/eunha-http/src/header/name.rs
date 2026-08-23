// MIT License
//
// Copyright (c) 2026 Tommaso Bruno
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

use std::error::Error;
use std::fmt::{self, Display, Formatter};

/// Returned when a field name is empty or contains a byte outside HTTP's token syntax.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InvalidHeaderName;

impl Display for InvalidHeaderName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        formatter.write_str("invalid HTTP header name")
    }
}

impl Error for InvalidHeaderName {}

/// A validated, lowercase HTTP field name.
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub struct HeaderName(String);

impl HeaderName {
    pub fn as_str(&self) -> &str {
        &self.0
    }

    fn is_valid(name: &[u8]) -> bool {
        !name.is_empty()
            && name.iter().copied().all(|byte| {
                byte.is_ascii_alphanumeric()
                    || matches!(
                        byte,
                        b'!' | b'#'
                            | b'$'
                            | b'%'
                            | b'&'
                            | b'\''
                            | b'*'
                            | b'+'
                            | b'-'
                            | b'.'
                            | b'^'
                            | b'_'
                            | b'`'
                            | b'|'
                            | b'~'
                    )
            })
    }
}

impl TryFrom<Vec<u8>> for HeaderName {
    type Error = InvalidHeaderName;

    fn try_from(mut name: Vec<u8>) -> Result<Self, Self::Error> {
        if !Self::is_valid(&name) {
            return Err(InvalidHeaderName);
        }

        name.make_ascii_lowercase();
        let name = String::from_utf8(name).map_err(|_| InvalidHeaderName)?;
        Ok(Self(name))
    }
}

impl Display for HeaderName {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        formatter.write_str(self.as_str())
    }
}

#[cfg(test)]
mod tests {
    use super::{HeaderName, InvalidHeaderName};

    fn name(value: &str) -> HeaderName {
        HeaderName::try_from(value.as_bytes().to_vec()).expect("test header name should be valid")
    }

    #[test]
    fn validates_and_normalizes_field_names() {
        assert_eq!(name("Content-Type").as_str(), "content-type");
        assert_eq!(name("X_Custom~Name").as_str(), "x_custom~name");

        assert_eq!(HeaderName::try_from(Vec::new()), Err(InvalidHeaderName));
        assert_eq!(
            HeaderName::try_from(b"bad name".to_vec()),
            Err(InvalidHeaderName)
        );
        assert_eq!(
            HeaderName::try_from(b"bad:name".to_vec()),
            Err(InvalidHeaderName)
        );
        assert_eq!(
            HeaderName::try_from("café".as_bytes().to_vec()),
            Err(InvalidHeaderName)
        );
    }
}
