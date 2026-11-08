#include "GraphVisualizer.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iomanip>

namespace crawler {

namespace {

std::string escapeHtml(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string escapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

} // anonymous namespace

std::string GraphVisualizer::toDot(const CrawlGraph& graph, const PageStorage& storage) {
    std::ostringstream dot;
    dot << "digraph CrawlGraph {\n";
    dot << "  rankdir=LR;\n";
    dot << "  bgcolor=\"#ffffff\";\n";
    dot << "  node [shape=box, style=\"filled,rounded\", fontname=\"Helvetica,Arial,sans-serif\", fontsize=10, margin=\"0.2,0.1\"];\n";
    dot << "  edge [color=\"#888888\", arrowsize=0.7, penwidth=1.2];\n\n";

    auto pages = storage.getAllPages();
    std::unordered_map<std::string, std::string> nodeIds;
    size_t idCounter = 0;

    for (const auto& page : pages) {
        std::string nid = "node_" + std::to_string(idCounter++);
        nodeIds[page.url] = nid;

        std::string fillColor = "#e8f5e9"; // 200 green
        std::string strokeColor = "#388e3c";
        if (page.statusCode >= 300 && page.statusCode < 400) {
            fillColor = "#e3f2fd"; strokeColor = "#1976d2"; // 3xx blue
        } else if (page.statusCode >= 400 && page.statusCode < 500) {
            fillColor = "#fff3e0"; strokeColor = "#f57c00"; // 4xx orange
        } else if (page.statusCode >= 500 || page.statusCode < 0) {
            fillColor = "#ffebee"; strokeColor = "#d32f2f"; // 5xx red
        }

        std::string displayLabel = page.title.empty() ? page.url : page.title;
        if (displayLabel.size() > 40) {
            displayLabel = displayLabel.substr(0, 37) + "...";
        }
        std::string labelText = displayLabel + "\\n[" + (page.statusCode < 0 ? "ERR" : std::to_string(page.statusCode)) + "] d=" + std::to_string(page.depth);

        dot << "  " << nid << " [label=\"" << escapeJson(labelText)
            << "\", fillcolor=\"" << fillColor
            << "\", color=\"" << strokeColor
            << "\", tooltip=\"" << escapeJson(page.url) << "\"];\n";
    }

    dot << "\n";
    for (const auto& page : pages) {
        auto fromIt = nodeIds.find(page.url);
        if (fromIt == nodeIds.end()) continue;

        auto outEdges = graph.getOutEdges(page.url);
        for (const auto& target : outEdges) {
            auto toIt = nodeIds.find(target);
            if (toIt != nodeIds.end()) {
                dot << "  " << fromIt->second << " -> " << toIt->second << ";\n";
            }
        }
    }

    dot << "}\n";
    return dot.str();
}

std::string GraphVisualizer::toMermaid(const CrawlGraph& graph, size_t maxNodes) {
    std::ostringstream mmd;
    mmd << "flowchart TD\n";

    auto deadEnds = graph.getDeadEndPages();
    auto authorities = graph.getMostLinkedPages(maxNodes);

    std::unordered_map<std::string, std::string> nodeIds;
    size_t idCounter = 0;

    for (const auto& [url, deg] : authorities) {
        if (idCounter >= maxNodes) break;
        std::string nid = "N" + std::to_string(idCounter++);
        nodeIds[url] = nid;
        std::string cleanLabel = url;
        if (cleanLabel.size() > 50) cleanLabel = cleanLabel.substr(0, 47) + "...";
        mmd << "  " << nid << "[\"" << cleanLabel << "\"]\n";
    }

    for (const auto& [url, nid] : nodeIds) {
        auto outEdges = graph.getOutEdges(url);
        for (const auto& target : outEdges) {
            auto targetIt = nodeIds.find(target);
            if (targetIt != nodeIds.end()) {
                mmd << "  " << nid << " --> " << targetIt->second << "\n";
            }
        }
    }

    return mmd.str();
}

std::string GraphVisualizer::toHtmlViewer(const CrawlGraph& graph,
                                         const PageStorage& storage,
                                         const std::string& title) {
    auto pages = storage.getAllPages();
    std::ostringstream jsonNodes;
    std::ostringstream jsonLinks;

    jsonNodes << "[\n";
    for (size_t i = 0; i < pages.size(); ++i) {
        const auto& p = pages[i];
        jsonNodes << "  {\"id\":" << i
                  << ",\"url\":\"" << escapeJson(p.url) << "\""
                  << ",\"title\":\"" << escapeJson(p.title) << "\""
                  << ",\"status\":" << p.statusCode
                  << ",\"depth\":" << p.depth
                  << ",\"length\":" << p.contentLength
                  << ",\"inDeg\":" << graph.inDegree(p.url)
                  << ",\"outDeg\":" << graph.outDegree(p.url)
                  << "}";
        if (i + 1 < pages.size()) jsonNodes << ",";
        jsonNodes << "\n";
    }
    jsonNodes << "]";

    std::unordered_map<std::string, size_t> urlToIndex;
    for (size_t i = 0; i < pages.size(); ++i) {
        urlToIndex[pages[i].url] = i;
    }

    jsonLinks << "[\n";
    bool firstLink = true;
    for (const auto& p : pages) {
        size_t sourceIdx = urlToIndex[p.url];
        auto targets = graph.getOutEdges(p.url);
        for (const auto& t : targets) {
            auto it = urlToIndex.find(t);
            if (it != urlToIndex.end()) {
                if (!firstLink) jsonLinks << ",\n";
                firstLink = false;
                jsonLinks << "  {\"source\":" << sourceIdx << ",\"target\":" << it->second << "}";
            }
        }
    }
    jsonLinks << "\n]";

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html << "<meta charset=\"UTF-8\"/>\n";
    html << "<title>" << escapeHtml(title) << "</title>\n";
    html << "<style>\n";
    html << "  * { box-sizing: border-box; margin: 0; padding: 0; }\n";
    html << "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #0f172a; color: #f8fafc; display: flex; height: 100vh; overflow: hidden; }\n";
    html << "  #sidebar { width: 340px; background: #1e293b; border-right: 1px solid #334155; display: flex; flex-direction: column; z-index: 10; }\n";
    html << "  .header { padding: 16px; border-bottom: 1px solid #334155; }\n";
    html << "  .header h1 { font-size: 1.1rem; color: #38bdf8; margin-bottom: 6px; }\n";
    html << "  .header p { font-size: 0.8rem; color: #94a3b8; }\n";
    html << "  .search-box { padding: 12px 16px; border-bottom: 1px solid #334155; }\n";
    html << "  .search-box input { width: 100%; padding: 8px 12px; background: #0f172a; border: 1px solid #475569; border-radius: 6px; color: #fff; font-size: 0.85rem; }\n";
    html << "  #details { padding: 16px; flex: 1; overflow-y: auto; font-size: 0.85rem; }\n";
    html << "  .detail-row { margin-bottom: 12px; }\n";
    html << "  .detail-row label { font-size: 0.75rem; text-transform: uppercase; color: #64748b; display: block; margin-bottom: 2px; }\n";
    html << "  .detail-row value { font-weight: 500; word-break: break-all; }\n";
    html << "  .status-badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-weight: 600; font-size: 0.8rem; }\n";
    html << "  .status-200 { background: #065f46; color: #34d399; }\n";
    html << "  .status-300 { background: #1e40af; color: #60a5fa; }\n";
    html << "  .status-400 { background: #854d0e; color: #facc15; }\n";
    html << "  .status-500 { background: #991b1b; color: #f87171; }\n";
    html << "  #main { flex: 1; position: relative; }\n";
    html << "  canvas { width: 100%; height: 100%; display: block; cursor: grab; }\n";
    html << "  canvas:active { cursor: grabbing; }\n";
    html << "  .legend { position: absolute; bottom: 16px; right: 16px; background: rgba(30,41,59,0.9); padding: 10px 14px; border-radius: 8px; border: 1px solid #334155; font-size: 0.75rem; }\n";
    html << "  .legend-item { display: flex; align-items: center; gap: 8px; margin-bottom: 4px; }\n";
    html << "  .legend-dot { width: 10px; height: 10px; border-radius: 50%; display: inline-block; }\n";
    html << "</style>\n</head>\n<body>\n";
    html << "<div id=\"sidebar\">\n";
    html << "  <div class=\"header\">\n";
    html << "    <h1>" << escapeHtml(title) << "</h1>\n";
    html << "    <p>Nodes: <b>" << pages.size() << "</b> | Edges: <b>" << graph.edgeCount() << "</b> | Cycles: <b>" << (graph.hasCycle() ? "Yes" : "None") << "</b></p>\n";
    html << "  </div>\n";
    html << "  <div class=\"search-box\">\n";
    html << "    <input type=\"text\" id=\"search\" placeholder=\"Search URL or title...\" oninput=\"onSearch()\"/>\n";
    html << "  </div>\n";
    html << "  <div id=\"details\">\n";
    html << "    <p style=\"color:#64748b;\">Click on any node in the canvas to view detailed crawler metadata.</p>\n";
    html << "  </div>\n";
    html << "</div>\n";
    html << "<div id=\"main\">\n";
    html << "  <canvas id=\"viewport\"></canvas>\n";
    html << "  <div class=\"legend\">\n";
    html << "    <div class=\"legend-item\"><span class=\"legend-dot\" style=\"background:#10b981;\"></span> 200 OK</div>\n";
    html << "    <div class=\"legend-item\"><span class=\"legend-dot\" style=\"background:#3b82f6;\"></span> 3xx Redirect</div>\n";
    html << "    <div class=\"legend-item\"><span class=\"legend-dot\" style=\"background:#f59e0b;\"></span> 4xx Client Error</div>\n";
    html << "    <div class=\"legend-item\"><span class=\"legend-dot\" style=\"background:#ef4444;\"></span> 5xx / Network Fail</div>\n";
    html << "  </div>\n";
    html << "</div>\n";

    html << "<script>\n";
    html << "const nodes = " << jsonNodes.str() << ";\n";
    html << "const links = " << jsonLinks.str() << ";\n";
    html << R"raw(
const canvas = document.getElementById('viewport');
const ctx = canvas.getContext('2d');
let width, height;
let panX = 0, panY = 0, zoom = 1.0;
let isDragging = false, startX, startY;
let selectedNode = null;
let searchQuery = "";

function resize() {
  width = canvas.width = canvas.clientWidth;
  height = canvas.height = canvas.clientHeight;
  draw();
}
window.addEventListener('resize', resize);

// Layout nodes in BFS depth columns
const depthGroups = {};
nodes.forEach(n => {
  if (!depthGroups[n.depth]) depthGroups[n.depth] = [];
  depthGroups[n.depth].push(n);
});

const colWidth = 260;
Object.keys(depthGroups).forEach(d => {
  const col = depthGroups[d];
  const totalH = col.length * 80;
  col.forEach((n, idx) => {
    n.x = 120 + d * colWidth;
    n.y = 120 + (idx * 80) - (totalH / 2) + 200;
  });
});

function draw() {
  ctx.clearRect(0, 0, width, height);
  ctx.save();
  ctx.translate(panX, panY);
  ctx.scale(zoom, zoom);

  // Draw links
  links.forEach(l => {
    const src = nodes[l.source];
    const tgt = nodes[l.target];
    if (!src || !tgt) return;

    ctx.strokeStyle = '#334155';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(src.x, src.y);
    const cpX = (src.x + tgt.x) / 2;
    ctx.quadraticCurveTo(cpX, src.y, tgt.x, tgt.y);
    ctx.stroke();

    // Arrow
    const angle = Math.atan2(tgt.y - src.y, tgt.x - src.x);
    ctx.fillStyle = '#64748b';
    ctx.beginPath();
    ctx.arc(tgt.x, tgt.y, 3, 0, 2*Math.PI);
    ctx.fill();
  });

  // Draw nodes
  nodes.forEach(n => {
    let color = '#10b981';
    if (n.status >= 300 && n.status < 400) color = '#3b82f6';
    else if (n.status >= 400 && n.status < 500) color = '#f59e0b';
    else if (n.status >= 500 || n.status < 0) color = '#ef4444';

    const isMatch = searchQuery && (n.url.toLowerCase().includes(searchQuery) || n.title.toLowerCase().includes(searchQuery));
    const isSel = selectedNode === n;

    ctx.fillStyle = color;
    ctx.beginPath();
    ctx.arc(n.x, n.y, isSel ? 10 : (isMatch ? 9 : 6), 0, 2*Math.PI);
    ctx.fill();

    if (isSel || isMatch) {
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2.5;
      ctx.stroke();
    }

    // Label
    ctx.font = '11px sans-serif';
    ctx.fillStyle = isSel ? '#38bdf8' : (isMatch ? '#f8fafc' : '#94a3b8');
    const label = n.title ? n.title : n.url.replace(/^https?:\/\//, '');
    ctx.fillText(label.length > 25 ? label.substr(0,22)+'...' : label, n.x + 10, n.y + 4);
  });

  ctx.restore();
}

canvas.addEventListener('mousedown', e => {
  isDragging = true;
  startX = e.clientX - panX;
  startY = e.clientY - panY;
});

window.addEventListener('mousemove', e => {
  if (isDragging) {
    panX = e.clientX - startX;
    panY = e.clientY - startY;
    draw();
  }
});

window.addEventListener('mouseup', () => { isDragging = false; });

canvas.addEventListener('wheel', e => {
  e.preventDefault();
  const factor = e.deltaY < 0 ? 1.1 : 0.9;
  zoom *= factor;
  zoom = Math.max(0.2, Math.min(3.0, zoom));
  draw();
});

canvas.addEventListener('click', e => {
  const rect = canvas.getBoundingClientRect();
  const clickX = (e.clientX - rect.left - panX) / zoom;
  const clickY = (e.clientY - rect.top - panY) / zoom;

  let clicked = null;
  nodes.forEach(n => {
    const dist = Math.hypot(n.x - clickX, n.y - clickY);
    if (dist <= 12) clicked = n;
  });

  if (clicked) {
    selectedNode = clicked;
    showDetails(clicked);
    draw();
  }
});

function showDetails(n) {
  let badgeClass = 'status-200';
  if (n.status >= 300 && n.status < 400) badgeClass = 'status-300';
  else if (n.status >= 400 && n.status < 500) badgeClass = 'status-400';
  else if (n.status >= 500 || n.status < 0) badgeClass = 'status-500';

  const details = document.getElementById('details');
  details.innerHTML = `
    <div class="detail-row">
      <label>HTTP Status</label>
      <span class="status-badge ${badgeClass}">${n.status < 0 ? 'FAIL' : n.status}</span>
    </div>
    <div class="detail-row">
      <label>URL</label>
      <value><a href="${n.url}" target="_blank" style="color:#38bdf8;">${n.url}</a></value>
    </div>
    <div class="detail-row">
      <label>Page Title</label>
      <value>${n.title ? n.title : '<i>(None)</i>'}</value>
    </div>
    <div class="detail-row">
      <label>Crawl Depth</label>
      <value>Level ${n.depth}</value>
    </div>
    <div class="detail-row">
      <label>Content Size</label>
      <value>${(n.length / 1024).toFixed(2)} KB (${n.length} bytes)</value>
    </div>
    <div class="detail-row">
      <label>Link Graph Degrees</label>
      <value>In-Degree: <b>${n.inDeg}</b> | Out-Degree: <b>${n.outDeg}</b></value>
    </div>
  `;
}

function onSearch() {
  searchQuery = document.getElementById('search').value.trim().toLowerCase();
  draw();
}

resize();
)raw";
    html << "</script>\n</body>\n</html>\n";

    return html.str();
}

bool GraphVisualizer::exportDotFile(const std::string& filepath,
                                   const CrawlGraph& graph,
                                   const PageStorage& storage) {
    std::ofstream out(filepath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << toDot(graph, storage);
    return out.good();
}

bool GraphVisualizer::exportMermaidFile(const std::string& filepath,
                                       const CrawlGraph& graph,
                                       size_t maxNodes) {
    std::ofstream out(filepath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << toMermaid(graph, maxNodes);
    return out.good();
}

bool GraphVisualizer::exportHtmlViewerFile(const std::string& filepath,
                                           const CrawlGraph& graph,
                                           const PageStorage& storage,
                                           const std::string& title) {
    std::ofstream out(filepath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << toHtmlViewer(graph, storage, title);
    return out.good();
}

} // namespace crawler

