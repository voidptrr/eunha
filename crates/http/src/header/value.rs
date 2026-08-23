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

/// Returned when a field value contains a forbidden control byte.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InvalidHeaderValue;

impl Display for InvalidHeaderValue {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        formatter.write_str("invalid HTTP header value")
    }
}

impl Error for InvalidHeaderValue {}

/// An HTTP field value containing only bytes permitted by the protocol.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct HeaderValue(Vec<u8>);

impl HeaderValue {
    pub fn as_bytes(&self) -> &[u8] {
        &self.0
    }

    fn is_valid(value: &[u8]) -> bool {
        value
            .iter()
            .all(|byte| *byte == b'\t' || (b' '..=b'~').contains(byte) || *byte >= 0x80)
    }
}

impl TryFrom<Vec<u8>> for HeaderValue {
    type Error = InvalidHeaderValue;

    fn try_from(value: Vec<u8>) -> Result<Self, Self::Error> {
        if !Self::is_valid(&value) {
            return Err(InvalidHeaderValue);
        }

        Ok(Self(value))
    }
}

#[cfg(test)]
mod tests {
    use super::{HeaderValue, InvalidHeaderValue};

    #[test]
    fn validates_field_values() {
        assert!(HeaderValue::try_from(Vec::new()).is_ok());
        assert!(HeaderValue::try_from(b"text/plain\t;charset=utf-8".to_vec()).is_ok());
        assert!(HeaderValue::try_from(vec![0x80, 0xff]).is_ok());

        assert_eq!(
            HeaderValue::try_from(b"line\r\nbreak".to_vec()),
            Err(InvalidHeaderValue)
        );
        assert_eq!(HeaderValue::try_from(vec![0]), Err(InvalidHeaderValue));
        assert_eq!(HeaderValue::try_from(vec![0x7f]), Err(InvalidHeaderValue));
    }
}
