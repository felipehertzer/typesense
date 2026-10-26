import { describe, expect, it } from "bun:test";
import { connect } from "node:net";
import { Phases } from "../src/constants";

/**
 * Sends a request whose body arrives in two parts, so that the server responds before it has read the whole
 * body, and then a `GET /health` on the same keep-alive connection. Resolves with everything the server sent.
 */
function earlyResponseThenHealth(head: string, body: string): Promise<string> {
  return new Promise((resolve) => {
    const socket = connect(8108, "localhost");
    let received = "";
    const done = () => {
      clearTimeout(timer);
      socket.destroy();
      resolve(received);
    };
    const timer = setTimeout(done, 3000);

    socket.setEncoding("utf8");
    socket.on("data", (chunk: string) => {
      received += chunk;
      if (received.includes('{"ok":true}')) {
        done();
      }
    });
    socket.on("close", done);
    socket.on("error", done);
    socket.on("connect", async () => {
      const half = Math.floor(body.length / 2);
      socket.write(head + body.slice(0, half));
      await new Promise((r) => setTimeout(r, 500));
      socket.write(body.slice(half));
      await new Promise((r) => setTimeout(r, 500));
      socket.write("GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n");
    });
  });
}

describe(Phases.SINGLE_FRESH, () => {
  const body = "x".repeat(32 * 1024);

  it.each([
    ["404", "POST /not-a-route HTTP/1.1\r\n"],
    ["401", "POST /collections/companies/documents/import HTTP/1.1\r\nX-TYPESENSE-API-KEY: wrong\r\n"],
  ])("serves the next request after a %s sent before the request body was read", async (status, requestLine) => {
    const received = await earlyResponseThenHealth(
      `${requestLine}Host: localhost\r\nContent-Type: text/plain\r\nContent-Length: ${body.length}\r\n\r\n`,
      body,
    );

    expect(received).toStartWith(`HTTP/1.1 ${status}`);
    expect(received).toContain('{"ok":true}');
  });
});
