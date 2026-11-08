# Crawler Safety, Politeness & Defense Architecture

This document describes the protective mechanisms implemented in the **Web Crawler with Deduplication** to prevent denial-of-service, protect internal network infrastructure, and defend against malicious crawler traps.

---

## 1. RFC 9309 Robots Exclusion Protocol

The crawler strictly adheres to the Internet Engineering Task Force (IETF) **RFC 9309 Robots Exclusion Protocol**:
- **Automatic Retrieval:** Fetches and parses `/robots.txt` before issuing crawl requests to a host.
- **Record Precedence:** Matches records specifically targeted to the crawler's configured `User-Agent` (`SearchBot/1.0`). If no specific group exists, it defaults to the wildcard `User-agent: *` section.
- **Pattern Matching:** Implements RFC 9309 prefix rules:
  - Wildcard `*` matching arbitrary character sequences.
  - End anchor `$` matching end of URL path.
  - Longest matching rule: when `Allow` and `Disallow` rules have equal length, `Allow` takes precedence.
- **Crawl-Delay Support:** Honors the `Crawl-delay` directive, spacing out successive requests to the same host.

---

## 2. Server-Side Request Forgery (SSRF) Guard

A common vulnerability in naive web crawlers is SSRF: an attacker instructs the crawler to query internal services, loopback interfaces, or cloud metadata endpoints.

### Defense Mechanism:
1. **Pre-Socket DNS Resolution:** The fetcher resolves the destination hostname to concrete IP addresses prior to opening any socket.
2. **Blocked IP Range Enforcement:**
   - **Loopback:** `127.0.0.0/8`, `::1`
   - **Private IPv4 (RFC 1918):**
     - `10.0.0.0/8`
     - `172.16.0.0/12` (`172.16.0.0` to `172.31.255.255`)
     - `192.168.0.0/16`
   - **Link-Local & Cloud Metadata:**
     - `169.254.0.0/16`
     - Specifically blocks `169.254.169.254` (AWS, Google Cloud, and Azure instance metadata services).
   - **Carrier-Grade NAT:** `100.64.0.0/10`
   - **IPv4-Mapped IPv6:** `::ffff:127.0.0.1`, `::ffff:169.254.169.254`, etc.
3. **Fail-Closed Principle:** If DNS resolution fails, or if an IP resolves to any restricted range, the request is immediately aborted with `StatusCode::SSRF_BLOCKED`.

---

## 3. Crawler Trap Detection

Malicious or poorly configured servers can generate an infinite number of unique URLs to trap search engine bots (e.g., infinite calendar pagination, recursive directories).

### Mitigations Implemented:
- **Repeating Path Segments:** The `TrapDetector` tokenizes the URL path. If any directory segment repeats more than 3 times (e.g., `/catalog/category/catalog/category/catalog/category/`), the URL is identified as a trap and dropped.
- **Maximum Path Depth:** Paths exceeding 16 segments or 512 characters are pruned.
- **Query Parameter Flooding:** URLs with more than 8 query parameters or repetitive tracking parameters are normalized or rejected.

---

## 4. Redirect Loop & Cycle Prevention

- **Max Redirect Threshold:** Bounded by `config.maxRedirects` (default: 10 hops).
- **Cycle Detection:** Maintains a set of URLs traversed during redirect resolution (e.g., `A -> B -> C -> A`). If a URL reappears in the redirect chain, the crawler detects an infinite loop and terminates with `StatusCode::REDIRECT_LOOP`.

---

## 5. Rate Limiting & Politeness Backoff

- **Per-Host Queuing:** Crawl tasks are metered on a per-domain basis with a configurable delay (default: 1000 ms between requests to the same hostname).
- **HTTP 429 & 503 Handling:** When a server responds with HTTP 429 ("Too Many Requests") or HTTP 503 ("Service Unavailable"):
  - Inspects the `Retry-After` header.
  - Automatically activates exponential backoff, doubling the delay up to a safe upper bound (30 seconds).
