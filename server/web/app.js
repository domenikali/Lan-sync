const API_BASE = "/api";

// Load file list on page load
window.onload = listFiles;

async function listFiles() {
  const res = await fetch(`${API_BASE}/files`);
  const data = await res.json();

  const ul = document.getElementById("fileList");
  ul.innerHTML = "";

  data.files.forEach(file => {
    const li = document.createElement("li");

    // File info
    const info = document.createElement("span");
    info.textContent = `${file.name} (${file.size} bytes)`;

    // Button container
    const btns = document.createElement("div");

    // Download button
    const downloadBtn = document.createElement("button");
    downloadBtn.textContent = "⬇️ Download";
    downloadBtn.onclick = () => downloadFile(file.name);

    // Delete button
    const deleteBtn = document.createElement("button");
    deleteBtn.textContent = "🗑️ Delete";
    deleteBtn.classList.add("delete");
    deleteBtn.onclick = () => deleteFile(file.name);

    btns.appendChild(downloadBtn);
    btns.appendChild(deleteBtn);

    li.appendChild(info);
    li.appendChild(btns);

    ul.appendChild(li);
  });
}

async function downloadFile(filename) {
  const res = await fetch(`${API_BASE}/download/${filename}`);
  if (!res.ok) {
    alert("Download failed");
    return;
  }
  const blob = await res.blob();
  const url = window.URL.createObjectURL(blob);

  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();

  window.URL.revokeObjectURL(url);
}

async function deleteFile(filename) {
  if (!confirm(`Delete "${filename}"?`)) return;

  const res = await fetch(`${API_BASE}/delete/${filename}`, { method: "DELETE" });
  if (res.ok) {
    listFiles();
  } else {
    alert("Delete failed");
  }
}

async function uploadFile() {
  const fileInput = document.getElementById("fileInput");
  const file = fileInput.files[0];
  if (!file) return;

  const res = await fetch(`${API_BASE}/upload`, {
    method: "POST",
    headers: {
      "X-Filename": file.name,
    },
    body: file
  });

  if (res.ok) {
    alert("Upload successful!");
    listFiles();
  } else {
    alert("Upload failed");
  }
}