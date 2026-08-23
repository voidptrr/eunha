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
use std::io::{self, ErrorKind};
use std::net::{SocketAddr, TcpListener, TcpStream};

use crate::config::ServerConfig;

#[derive(Debug)]
pub enum ServerError {
    Bind {
        address: SocketAddr,
        source: io::Error,
    },
    LocalAddress {
        source: io::Error,
    },
    Accept {
        source: io::Error,
    },
}

impl Display for ServerError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> fmt::Result {
        match self {
            Self::Bind { address, source } => {
                write!(
                    formatter,
                    "failed to bind TCP listener to {address}: {source}"
                )
            }
            Self::LocalAddress { source } => {
                write!(
                    formatter,
                    "failed to read the TCP listener address: {source}"
                )
            }
            Self::Accept { source } => {
                write!(formatter, "failed to accept a TCP connection: {source}")
            }
        }
    }
}

impl Error for ServerError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        match self {
            Self::Bind { source, .. } | Self::LocalAddress { source } | Self::Accept { source } => {
                Some(source)
            }
        }
    }
}

#[derive(Debug)]
pub struct Server {
    listener: TcpListener,
}

impl Server {
    pub fn bind(config: &ServerConfig) -> Result<Self, ServerError> {
        let address = SocketAddr::from(config);
        let listener = match TcpListener::bind(address) {
            Ok(listener) => listener,
            Err(source) => return Err(ServerError::Bind { address, source }),
        };

        Ok(Self { listener })
    }

    pub fn local_addr(&self) -> Result<SocketAddr, ServerError> {
        match self.listener.local_addr() {
            Ok(address) => Ok(address),
            Err(source) => Err(ServerError::LocalAddress { source }),
        }
    }

    fn accept(&self) -> Result<(TcpStream, SocketAddr), ServerError> {
        loop {
            match self.listener.accept() {
                Ok(connection) => return Ok(connection),
                Err(error) if error.kind() == ErrorKind::Interrupted => {}
                Err(source) => return Err(ServerError::Accept { source }),
            }
        }
    }

    /// Accepts and handles connections sequentially on the current thread.
    pub fn listen<H>(self, mut handler: H) -> Result<(), ServerError>
    where
        H: FnMut(TcpStream, SocketAddr),
    {
        loop {
            match self.accept() {
                Ok((stream, peer_address)) => handler(stream, peer_address),
                Err(error) => return Err(error),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use std::error::Error;
    use std::net::{IpAddr, Ipv4Addr, TcpStream};
    use std::thread;

    use super::{Server, ServerError};
    use crate::config::ServerConfigBuilder;

    fn config(port: u16) -> crate::ServerConfig {
        let mut builder = ServerConfigBuilder::new();
        builder.address(IpAddr::V4(Ipv4Addr::LOCALHOST)).port(port);
        builder.build().expect("complete config should build")
    }

    #[test]
    fn accepts_a_tcp_connection() {
        let server = Server::bind(&config(0)).expect("server should bind");
        let address = server.local_addr().expect("server should have an address");
        let client = thread::spawn(move || TcpStream::connect(address));

        let (_, peer_address) = server.accept().expect("server should accept a connection");
        client
            .join()
            .expect("client thread should finish")
            .expect("client should connect");

        assert_eq!(peer_address.ip(), IpAddr::V4(Ipv4Addr::LOCALHOST));
    }

    #[test]
    fn reports_bind_failures_with_their_source() {
        let first_server = Server::bind(&config(0)).expect("first server should bind");
        let address = first_server
            .local_addr()
            .expect("first server should have an address");
        let error = Server::bind(&config(address.port())).expect_err("address should be in use");

        assert!(matches!(
            &error,
            ServerError::Bind {
                address: failed_address,
                ..
            } if *failed_address == address
        ));
        assert!(
            error
                .to_string()
                .starts_with(&format!("failed to bind TCP listener to {address}: "))
        );
        assert!(error.source().is_some());
    }
}
