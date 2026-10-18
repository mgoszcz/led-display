const DEFAULT_MATRIX_WIDTH = 16;
const DEFAULT_MATRIX_HEIGHT = 16;

const colorInput = document.getElementById("colorInput");
const matrixSizeInput = document.getElementById("matrixSizeInput");
const textInput = document.getElementById("textInput");
const textSpeedInput = document.getElementById("textSpeedInput");
const brightnessInput = document.getElementById("brightnessInput");
const brightnessValue = document.getElementById("brightnessValue");
const clearButton = document.getElementById("clearButton");
const saveButton = document.getElementById("saveButton");
const loadButton = document.getElementById("loadButton");
const exportButton = document.getElementById("exportButton");
const importButton = document.getElementById("importButton");
const importInput = document.getElementById("importInput");
const imageModeInput = document.getElementById("imageModeInput");
const imageImportButton = document.getElementById("imageImportButton");
const imageInput = document.getElementById("imageInput");
const animationFrameInput = document.getElementById("animationFrameInput");
const animationSpeedInput = document.getElementById("animationSpeedInput");
const animationLoopInput = document.getElementById("animationLoopInput");
const addFrameButton = document.getElementById("addFrameButton");
const duplicateFrameButton = document.getElementById("duplicateFrameButton");
const deleteFrameButton = document.getElementById("deleteFrameButton");
const clearAnimationButton = document.getElementById("clearAnimationButton");
const selectButton = document.getElementById("selectButton");
const clearSelectionButton = document.getElementById("clearSelectionButton");
const shiftLeftButton = document.getElementById("shiftLeftButton");
const shiftRightButton = document.getElementById("shiftRightButton");
const shiftUpButton = document.getElementById("shiftUpButton");
const shiftDownButton = document.getElementById("shiftDownButton");
const previewAnimationButton = document.getElementById(
  "previewAnimationButton",
);
const sendAnimationButton = document.getElementById("sendAnimationButton");
const exportAnimationButton = document.getElementById("exportAnimationButton");
const importAnimationButton = document.getElementById("importAnimationButton");
const animationImportInput = document.getElementById("animationImportInput");
const demoButton = document.getElementById("demoButton");
const textButton = document.getElementById("textButton");
const sendButton = document.getElementById("sendButton");
const statusEl = document.getElementById("status");
const gridEl = document.getElementById("grid");

const DOCUMENT_FORMAT = "led-image";
const DOCUMENT_VERSION = 1;
const DOCUMENT_TYPE = "image";
const ANIMATION_DOCUMENT_FORMAT = "led-animation";
const ANIMATION_DOCUMENT_TYPE = "animation";
const MAX_ANIMATION_FRAMES = 20;
const STORAGE_KEY = "led-matrix-pixel-editor-frame";
let matrixWidth = DEFAULT_MATRIX_WIDTH;
let matrixHeight = DEFAULT_MATRIX_HEIGHT;
let pixels = createEmptyPixels(matrixWidth, matrixHeight);
let animationFrames = [pixels];
let activeFrameIndex = 0;
let previewTimer = null;
let previewFrameIndex = 0;
let isPointerDown = false;
let eraseMode = false;
let isSelectMode = false;
let isSelecting = false;
let selectionAnchorIndex = null;
let selectedPixels = new Set();
let framePixels = createEmptyPixels(matrixWidth, matrixHeight);
let currentView = "home";
let drawingTool = "draw";

function createEmptyPixels(width, height) {
  return Array.from({ length: width * height }, () => ({ r: 0, g: 0, b: 0 }));
}

function clonePixels(sourcePixels) {
  return sourcePixels.map((pixel) => ({ ...pixel }));
}

function getFrameBytesLength() {
  return matrixWidth * matrixHeight * 3;
}

function setStatus(message, type = "") {
  statusEl.textContent = message;
  statusEl.className = `status ${type}`.trim();
}

function getMatrixSizeValue(width, height) {
  return `${width}x${height}`;
}

function hexToRgb(hex) {
  const value = hex.replace("#", "");
  return {
    r: parseInt(value.slice(0, 2), 16),
    g: parseInt(value.slice(2, 4), 16),
    b: parseInt(value.slice(4, 6), 16),
  };
}

function colorToCss(color) {
  return `rgb(${color.r}, ${color.g}, ${color.b})`;
}

function updateBrightnessValue() {
  brightnessValue.textContent = `${brightnessInput.value}%`;
}

function setPixel(index, color) {
  pixels[index] = { ...color };
  gridEl.children[index].style.background = colorToCss(color);
}

function applyPixels(nextPixels) {
  for (let i = 0; i < pixels.length; i++) {
    setPixel(i, nextPixels[i]);
  }
}

function renderPixelsToGrid(framePixels) {
  for (let i = 0; i < framePixels.length; i++) {
    gridEl.children[i].style.background = colorToCss(framePixels[i]);
  }
  updateSelectionStyles();
}

function stopAnimationPreview() {
  if (previewTimer !== null) {
    clearInterval(previewTimer);
    previewTimer = null;
  }
  previewAnimationButton.textContent = "PLAY";
  renderPixelsToGrid(pixels);
}

function updateAnimationFrameSelector() {
  animationFrameInput.innerHTML = "";

  for (let i = 0; i < animationFrames.length; i++) {
    const option = document.createElement("option");
    option.value = String(i);
    option.textContent = `${i + 1} / ${animationFrames.length}`;
    animationFrameInput.appendChild(option);
  }

  animationFrameInput.value = String(activeFrameIndex);
}

function indexToPoint(index) {
  return {
    x: index % matrixWidth,
    y: Math.floor(index / matrixWidth),
  };
}

function updateSelectionStyles() {
  for (let i = 0; i < gridEl.children.length; i++) {
    gridEl.children[i].classList.toggle("selected", selectedPixels.has(i));
  }
}

function clearSelection() {
  selectedPixels = new Set();
  selectionAnchorIndex = null;
  isSelecting = false;
  updateSelectionStyles();
}

function setSelectMode(enabled) {
  stopAnimationPreview();
  isSelectMode = enabled;
  selectButton.classList.toggle("active", isSelectMode);

  if (!isSelectMode) {
    isSelecting = false;
  }

  setStatus(
    isSelectMode ? "Selection mode enabled." : "Selection mode disabled.",
    "ok",
  );
}

function updateRectSelection(fromIndex, toIndex) {
  const from = indexToPoint(fromIndex);
  const to = indexToPoint(toIndex);
  const minX = Math.min(from.x, to.x);
  const maxX = Math.max(from.x, to.x);
  const minY = Math.min(from.y, to.y);
  const maxY = Math.max(from.y, to.y);
  const nextSelection = new Set();

  for (let y = minY; y <= maxY; y++) {
    for (let x = minX; x <= maxX; x++) {
      nextSelection.add(y * matrixWidth + x);
    }
  }

  selectedPixels = nextSelection;
  updateSelectionStyles();
}

function setActiveAnimationFrame(index) {
  stopAnimationPreview();
  clearSelection();

  if (index < 0 || index >= animationFrames.length) {
    return;
  }

  activeFrameIndex = index;
  pixels = animationFrames[activeFrameIndex];
  updateAnimationFrameSelector();
  renderPixelsToGrid(pixels);
  setStatus(
    `Editing animation frame ${activeFrameIndex + 1} / ${animationFrames.length}.`,
    "ok",
  );
}

function resetAnimationFrames() {
  pixels = createEmptyPixels(matrixWidth, matrixHeight);
  animationFrames = [pixels];
  activeFrameIndex = 0;
  clearSelection();
  updateAnimationFrameSelector();
}

function addAnimationFrame() {
  stopAnimationPreview();

  if (animationFrames.length >= MAX_ANIMATION_FRAMES) {
    setStatus(`Animation frame limit is ${MAX_ANIMATION_FRAMES}.`, "error");
    return;
  }

  animationFrames.splice(
    activeFrameIndex + 1,
    0,
    createEmptyPixels(matrixWidth, matrixHeight),
  );
  setActiveAnimationFrame(activeFrameIndex + 1);
}

function duplicateAnimationFrame() {
  stopAnimationPreview();

  if (animationFrames.length >= MAX_ANIMATION_FRAMES) {
    setStatus(`Animation frame limit is ${MAX_ANIMATION_FRAMES}.`, "error");
    return;
  }

  animationFrames.splice(activeFrameIndex + 1, 0, clonePixels(pixels));
  setActiveAnimationFrame(activeFrameIndex + 1);
}

function deleteAnimationFrame() {
  stopAnimationPreview();

  if (animationFrames.length === 1) {
    setStatus("Animation must have at least one frame.", "error");
    return;
  }

  animationFrames.splice(activeFrameIndex, 1);
  setActiveAnimationFrame(
    Math.min(activeFrameIndex, animationFrames.length - 1),
  );
}

function shiftCurrentFrame(dx, dy) {
  stopAnimationPreview();

  if (selectedPixels.size > 0) {
    shiftSelection(dx, dy);
    return;
  }

  const shifted = createEmptyPixels(matrixWidth, matrixHeight);

  for (let y = 0; y < matrixHeight; y++) {
    for (let x = 0; x < matrixWidth; x++) {
      const sourceX = x - dx;
      const sourceY = y - dy;

      if (
        sourceX >= 0 &&
        sourceX < matrixWidth &&
        sourceY >= 0 &&
        sourceY < matrixHeight
      ) {
        shifted[y * matrixWidth + x] = {
          ...pixels[sourceY * matrixWidth + sourceX],
        };
      }
    }
  }

  applyPixels(shifted);
  setStatus(`Frame shifted ${dx}, ${dy}.`, "ok");
}

function shiftSelection(dx, dy) {
  const nextPixels = clonePixels(pixels);
  const nextSelection = new Set();

  for (const index of selectedPixels) {
    nextPixels[index] = { r: 0, g: 0, b: 0 };
  }

  for (const index of selectedPixels) {
    const point = indexToPoint(index);
    const nextX = point.x + dx;
    const nextY = point.y + dy;

    if (
      nextX < 0 ||
      nextX >= matrixWidth ||
      nextY < 0 ||
      nextY >= matrixHeight
    ) {
      continue;
    }

    const nextIndex = nextY * matrixWidth + nextX;
    nextPixels[nextIndex] = { ...pixels[index] };
    nextSelection.add(nextIndex);
  }

  selectedPixels = nextSelection;
  applyPixels(nextPixels);
  updateSelectionStyles();
  setStatus(`Selection shifted ${dx}, ${dy}.`, "ok");
}

function paintCell(cell, erase) {
  stopAnimationPreview();
  const index = Number(cell.dataset.index);
  const color = erase ? { r: 0, g: 0, b: 0 } : hexToRgb(colorInput.value);
  setPixel(index, color);
}

function buildFrameBytes() {
  const bytes = new Uint8Array(getFrameBytesLength());

  for (let i = 0; i < pixels.length; i++) {
    bytes[i * 3 + 0] = pixels[i].r;
    bytes[i * 3 + 1] = pixels[i].g;
    bytes[i * 3 + 2] = pixels[i].b;
  }

  return bytes;
}

function buildAnimationBytes() {
  const frameBytesLength = getFrameBytesLength();
  const bytes = new Uint8Array(frameBytesLength * animationFrames.length);

  for (let frameIndex = 0; frameIndex < animationFrames.length; frameIndex++) {
    const frame = animationFrames[frameIndex];
    const frameOffset = frameIndex * frameBytesLength;

    for (let pixelIndex = 0; pixelIndex < frame.length; pixelIndex++) {
      bytes[frameOffset + pixelIndex * 3 + 0] = frame[pixelIndex].r;
      bytes[frameOffset + pixelIndex * 3 + 1] = frame[pixelIndex].g;
      bytes[frameOffset + pixelIndex * 3 + 2] = frame[pixelIndex].b;
    }
  }

  return bytes;
}

function getAnimationSpeedMs() {
  const speed = Number(animationSpeedInput.value);

  if (!Number.isInteger(speed) || speed < 10 || speed > 5000) {
    throw new Error("Animation speed must be an integer from 10 to 5000 ms.");
  }

  return speed;
}

function createDrawingSnapshot() {
  return {
    format: DOCUMENT_FORMAT,
    version: DOCUMENT_VERSION,
    type: DOCUMENT_TYPE,
    width: matrixWidth,
    height: matrixHeight,
    brightness: Number(brightnessInput.value),
    pixels: pixels.map((pixel) => [pixel.r, pixel.g, pixel.b]),
  };
}

function createAnimationSnapshot() {
  return {
    format: ANIMATION_DOCUMENT_FORMAT,
    version: DOCUMENT_VERSION,
    type: ANIMATION_DOCUMENT_TYPE,
    width: matrixWidth,
    height: matrixHeight,
    frameDurationMs: getAnimationSpeedMs(),
    loop: animationLoopInput.value === "1",
    frames: animationFrames.map((frame) =>
      frame.map((pixel) => [pixel.r, pixel.g, pixel.b]),
    ),
  };
}

function parsePixelArray(rawPixels, expectedLength) {
  if (!Array.isArray(rawPixels) || rawPixels.length !== expectedLength) {
    throw new Error(`Expected ${expectedLength} pixels.`);
  }

  return rawPixels.map((pixel) => {
    if (!Array.isArray(pixel) || pixel.length !== 3) {
      throw new Error("Invalid pixel data.");
    }

    const [r, g, b] = pixel.map((channel) => Number(channel));
    if (
      ![r, g, b].every(
        (channel) =>
          Number.isInteger(channel) && channel >= 0 && channel <= 255,
      )
    ) {
      throw new Error("Pixel channel must be an integer from 0 to 255.");
    }

    return { r, g, b };
  });
}

function parseDrawingSnapshot(value) {
  const snapshot = typeof value === "string" ? JSON.parse(value) : value;

  if (!snapshot || typeof snapshot !== "object") {
    throw new Error("Invalid drawing document.");
  }

  if (snapshot.format !== undefined && snapshot.format !== DOCUMENT_FORMAT) {
    throw new Error(`Expected ${DOCUMENT_FORMAT} format.`);
  }

  if (snapshot.version !== DOCUMENT_VERSION) {
    throw new Error(`Expected document version ${DOCUMENT_VERSION}.`);
  }

  if (snapshot.type !== undefined && snapshot.type !== DOCUMENT_TYPE) {
    throw new Error(`Expected ${DOCUMENT_TYPE} document type.`);
  }

  if (
    !snapshot ||
    snapshot.width !== matrixWidth ||
    snapshot.height !== matrixHeight
  ) {
    throw new Error(`Expected ${matrixWidth}x${matrixHeight} drawing.`);
  }

  return {
    brightness: Number.isInteger(snapshot.brightness)
      ? snapshot.brightness
      : null,
    pixels: parsePixelArray(snapshot.pixels, pixels.length),
  };
}

function parseAnimationSnapshot(value) {
  const snapshot = typeof value === "string" ? JSON.parse(value) : value;

  if (!snapshot || typeof snapshot !== "object") {
    throw new Error("Invalid animation document.");
  }

  if (snapshot.format !== ANIMATION_DOCUMENT_FORMAT) {
    throw new Error(`Expected ${ANIMATION_DOCUMENT_FORMAT} format.`);
  }

  if (snapshot.version !== DOCUMENT_VERSION) {
    throw new Error(`Expected document version ${DOCUMENT_VERSION}.`);
  }

  if (
    snapshot.type !== undefined &&
    snapshot.type !== ANIMATION_DOCUMENT_TYPE
  ) {
    throw new Error(`Expected ${ANIMATION_DOCUMENT_TYPE} document type.`);
  }

  if (snapshot.width !== matrixWidth || snapshot.height !== matrixHeight) {
    throw new Error(`Expected ${matrixWidth}x${matrixHeight} animation.`);
  }

  if (
    !Array.isArray(snapshot.frames) ||
    snapshot.frames.length < 1 ||
    snapshot.frames.length > MAX_ANIMATION_FRAMES
  ) {
    throw new Error(`Animation must have 1-${MAX_ANIMATION_FRAMES} frames.`);
  }

  const frameDurationMs = Number(snapshot.frameDurationMs);
  if (
    !Number.isInteger(frameDurationMs) ||
    frameDurationMs < 10 ||
    frameDurationMs > 5000
  ) {
    throw new Error("Animation speed must be an integer from 10 to 5000 ms.");
  }

  return {
    frameDurationMs,
    loop: Boolean(snapshot.loop),
    frames: snapshot.frames.map((frame) =>
      parsePixelArray(frame, pixels.length),
    ),
  };
}

function applyDrawingSnapshot(snapshot) {
  stopAnimationPreview();
  clearSelection();
  applyPixels(snapshot.pixels);

  if (snapshot.brightness !== null) {
    brightnessInput.value = String(
      Math.max(1, Math.min(100, snapshot.brightness)),
    );
    updateBrightnessValue();
  }
}

function applyAnimationSnapshot(snapshot) {
  stopAnimationPreview();
  clearSelection();
  animationFrames = snapshot.frames;
  activeFrameIndex = 0;
  pixels = animationFrames[activeFrameIndex];
  animationSpeedInput.value = String(snapshot.frameDurationMs);
  animationLoopInput.value = snapshot.loop ? "1" : "0";
  updateAnimationFrameSelector();
  renderPixelsToGrid(pixels);
}

function clearGrid() {
  stopAnimationPreview();
  clearSelection();

  for (let i = 0; i < pixels.length; i++) {
    setPixel(i, { r: 0, g: 0, b: 0 });
  }
  setStatus("Grid cleared.");
}

function clearAnimation() {
  stopAnimationPreview();
  resetAnimationFrames();
  renderPixelsToGrid(pixels);
  setStatus("Animation cleared.", "ok");
}

function saveDrawing() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(createDrawingSnapshot()));
  setStatus("Drawing saved in this browser.", "ok");
}

async function loadDrawing() {
  const saved = localStorage.getItem(STORAGE_KEY);
  if (!saved) {
    setStatus("No saved drawing found.", "error");
    return;
  }

  try {
    applyDrawingSnapshot(parseDrawingSnapshot(saved));
    const brightnessSent = await sendBrightness();
    if (brightnessSent) {
      setStatus("Drawing loaded from this browser.", "ok");
    } else {
      setStatus("Drawing loaded, but brightness was not sent.", "error");
    }
  } catch (error) {
    setStatus(`Load failed: ${error.message}`, "error");
  }
}

function exportDrawing() {
  const content = JSON.stringify(createDrawingSnapshot(), null, 2);
  const blob = new Blob([content], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  const timestamp = new Date().toISOString().replace(/[:.]/g, "-");

  link.href = url;
  link.download = `led-image-${matrixWidth}x${matrixHeight}-${timestamp}.json`;
  link.click();
  URL.revokeObjectURL(url);
  setStatus("Drawing exported as JSON.", "ok");
}

async function importDrawing(file) {
  if (!file) {
    return;
  }

  try {
    const content = await file.text();
    applyDrawingSnapshot(parseDrawingSnapshot(content));
    const brightnessSent = await sendBrightness();
    if (brightnessSent) {
      setStatus(`Imported ${file.name}.`, "ok");
    } else {
      setStatus(`Imported ${file.name}, but brightness was not sent.`, "error");
    }
  } catch (error) {
    setStatus(`Import failed: ${error.message}`, "error");
  } finally {
    importInput.value = "";
  }
}

function exportAnimation() {
  try {
    const content = JSON.stringify(createAnimationSnapshot(), null, 2);
    const blob = new Blob([content], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    const timestamp = new Date().toISOString().replace(/[:.]/g, "-");

    link.href = url;
    link.download = `led-animation-${matrixWidth}x${matrixHeight}-${animationFrames.length}frames-${timestamp}.json`;
    link.click();
    URL.revokeObjectURL(url);
    setStatus("Animation exported as JSON.", "ok");
  } catch (error) {
    setStatus(`Animation export failed: ${error.message}`, "error");
  }
}

async function importAnimation(file) {
  if (!file) {
    return;
  }

  try {
    const content = await file.text();
    applyAnimationSnapshot(parseAnimationSnapshot(content));
    setStatus(`Imported animation ${file.name}.`, "ok");
  } catch (error) {
    setStatus(`Animation import failed: ${error.message}`, "error");
  } finally {
    animationImportInput.value = "";
  }
}

function loadImageFile(file) {
  return new Promise((resolve, reject) => {
    const url = URL.createObjectURL(file);
    const image = new Image();

    image.onload = () => {
      URL.revokeObjectURL(url);
      resolve(image);
    };

    image.onerror = () => {
      URL.revokeObjectURL(url);
      reject(new Error("Failed to load image file."));
    };

    image.src = url;
  });
}

function getImageDrawRect(image, mode) {
  const imageRatio = image.naturalWidth / image.naturalHeight;
  const canvasRatio = matrixWidth / matrixHeight;

  if (mode === "fit") {
    if (imageRatio > canvasRatio) {
      const width = matrixWidth;
      const height = matrixWidth / imageRatio;
      return { dx: 0, dy: (matrixHeight - height) / 2, width, height };
    }

    const height = matrixHeight;
    const width = matrixHeight * imageRatio;
    return { dx: (matrixWidth - width) / 2, dy: 0, width, height };
  }

  if (imageRatio > canvasRatio) {
    const height = matrixHeight;
    const width = matrixHeight * imageRatio;
    return { dx: (matrixWidth - width) / 2, dy: 0, width, height };
  }

  const width = matrixWidth;
  const height = matrixWidth / imageRatio;
  return { dx: 0, dy: (matrixHeight - height) / 2, width, height };
}

function convertImageToPixels(image) {
  const canvas = document.createElement("canvas");
  canvas.width = matrixWidth;
  canvas.height = matrixHeight;

  const context = canvas.getContext("2d", { willReadFrequently: true });
  const drawRect = getImageDrawRect(image, imageModeInput.value);

  context.fillStyle = "rgb(0, 0, 0)";
  context.fillRect(0, 0, matrixWidth, matrixHeight);
  context.imageSmoothingEnabled = true;
  context.imageSmoothingQuality = "high";
  context.drawImage(
    image,
    drawRect.dx,
    drawRect.dy,
    drawRect.width,
    drawRect.height,
  );

  const imageData = context.getImageData(0, 0, matrixWidth, matrixHeight).data;
  const nextPixels = [];

  for (let i = 0; i < imageData.length; i += 4) {
    nextPixels.push({
      r: imageData[i],
      g: imageData[i + 1],
      b: imageData[i + 2],
    });
  }

  clearSelection();
  applyPixels(nextPixels);
}

async function importImage(file) {
  if (!file) {
    return;
  }

  try {
    const image = await loadImageFile(file);
    convertImageToPixels(image);
    setStatus(
      `Converted ${file.name} to ${matrixWidth}x${matrixHeight}.`,
      "ok",
    );
  } catch (error) {
    setStatus(`Image import failed: ${error.message}`, "error");
  } finally {
    imageInput.value = "";
  }
}

async function loadDisplayConfig() {
  const response = await fetch("/display", { method: "GET" });
  const text = await response.text();

  if (!response.ok) {
    throw new Error(`Display config failed: ${response.status} ${text}`);
  }

  let config;
  try {
    config = JSON.parse(text);
  } catch {
    throw new Error("Display config response is not valid JSON.");
  }

  const width = Number(config.width);
  const height = Number(config.height);

  if (
    !Number.isInteger(width) ||
    !Number.isInteger(height) ||
    width <= 0 ||
    height <= 0
  ) {
    throw new Error("Display config has invalid width or height.");
  }

  const sizeValue = getMatrixSizeValue(width, height);
  const optionExists = Array.from(matrixSizeInput.options).some(
    (option) => option.value === sizeValue,
  );

  if (!optionExists) {
    throw new Error(`Unsupported display size from ESP32: ${sizeValue}.`);
  }

  setMatrixSize(width, height, false);
  document.getElementById("displayInfo").textContent = sizeValue;
  setStatus(`ESP32 health check OK. Display size: ${sizeValue}.`, "ok");
}

async function sendFrame() {
  stopAnimationPreview();

  try {
    const response = await fetch("/frame", {
      method: "POST",
      body: buildFrameBytes(),
    });

    const text = await response.text();
    if (response.ok) {
      setStatus(`Frame sent: ${text || "OK"}`, "ok");
    } else {
      setStatus(`Frame failed: ${response.status} ${text}`, "error");
    }
  } catch (error) {
    setStatus(`Frame request failed: ${error.message}`, "error");
  }
}

function toggleAnimationPreview() {
  if (previewTimer !== null) {
    stopAnimationPreview();
    setStatus(
      `Editing animation frame ${activeFrameIndex + 1} / ${animationFrames.length}.`,
      "ok",
    );
    return;
  }

  try {
    const speed = getAnimationSpeedMs();
    const shouldLoop = animationLoopInput.value === "1";
    previewFrameIndex = activeFrameIndex;
    previewAnimationButton.textContent = "STOP";
    renderPixelsToGrid(animationFrames[previewFrameIndex]);

    previewTimer = setInterval(() => {
      if (!shouldLoop && previewFrameIndex >= animationFrames.length - 1) {
        stopAnimationPreview();
        setStatus("Animation preview finished.", "ok");
        return;
      }

      previewFrameIndex = shouldLoop
        ? (previewFrameIndex + 1) % animationFrames.length
        : previewFrameIndex + 1;
      renderPixelsToGrid(animationFrames[previewFrameIndex]);
    }, speed);

    setStatus(`Previewing ${animationFrames.length} animation frame(s).`, "ok");
  } catch (error) {
    setStatus(`Preview failed: ${error.message}`, "error");
  }
}

async function sendAnimation() {
  stopAnimationPreview();

  try {
    const speed = getAnimationSpeedMs();
    const loop = animationLoopInput.value;
    const response = await fetch(
      `/animation?frames=${animationFrames.length}&speedMs=${speed}&loop=${loop}`,
      {
        method: "POST",
        headers: {
          "Content-Type": "application/octet-stream",
        },
        body: buildAnimationBytes(),
      },
    );

    const text = await response.text();
    if (response.ok) {
      setStatus(`Animation sent: ${text || "OK"}`, "ok");
    } else {
      setStatus(`Animation failed: ${response.status} ${text}`, "error");
    }
  } catch (error) {
    setStatus(`Animation request failed: ${error.message}`, "error");
  }
}

async function sendBrightness() {
  updateBrightnessValue();

  try {
    const response = await fetch("/brightness", {
      method: "POST",
      body: brightnessInput.value,
    });

    const text = await response.text();
    if (response.ok) {
      setStatus(
        `Brightness set to ${brightnessInput.value}%: ${text || "OK"}`,
        "ok",
      );
      return true;
    } else {
      setStatus(`Brightness failed: ${response.status} ${text}`, "error");
      return false;
    }
  } catch (error) {
    setStatus(`Brightness request failed: ${error.message}`, "error");
    return false;
  }
}

async function enableDemo() {
  try {
    const response = await fetch("/demo", { method: "POST" });
    const text = await response.text();

    if (response.ok) {
      setStatus(`Demo enabled: ${text || "OK"}`, "ok");
    } else {
      setStatus(`Demo failed: ${response.status} ${text}`, "error");
    }
  } catch (error) {
    setStatus(`Demo request failed: ${error.message}`, "error");
  }
}

function getTextSpeedMs() {
  const speed = Number(textSpeedInput.value);

  if (!Number.isInteger(speed) || speed < 10 || speed > 5000) {
    throw new Error("Text speed must be an integer from 10 to 5000 ms.");
  }

  return speed;
}

async function sendText() {
  const text = textInput.value.trim();

  if (!text) {
    setStatus("Enter text before sending.", "error");
    return;
  }

  try {
    const payload = {
      text,
      color: hexToRgb(document.getElementById("textColorInput").value),
      speedMs: getTextSpeedMs(),
    };

    const response = await fetch("/text", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(payload),
    });

    const responseText = await response.text();
    if (response.ok) {
      setStatus(`Text sent: ${responseText || "OK"}`, "ok");
    } else {
      setStatus(`Text failed: ${response.status} ${responseText}`, "error");
    }
  } catch (error) {
    setStatus(`Text request failed: ${error.message}`, "error");
  }
}

function createGrid() {
  gridEl.innerHTML = "";
  gridEl.style.setProperty("--matrix-width", String(matrixWidth));
  gridEl.style.setProperty("--matrix-height", String(matrixHeight));
  gridEl.setAttribute(
    "aria-label",
    `${matrixWidth} by ${matrixHeight} pixel grid`,
  );

  for (let i = 0; i < matrixWidth * matrixHeight; i++) {
    const cell = document.createElement("button");
    cell.type = "button";
    cell.className = "cell";
    cell.dataset.index = String(i);
    cell.setAttribute("aria-label", `Pixel ${i}`);

    cell.addEventListener("contextmenu", (event) => {
      event.preventDefault();
    });

    cell.addEventListener("pointerdown", (event) => {
      event.preventDefault();
      isPointerDown = true;
      eraseMode = event.button === 2 || drawingTool === "erase";

      if (isSelectMode) {
        isSelecting = true;
        selectionAnchorIndex = Number(cell.dataset.index);
        updateRectSelection(selectionAnchorIndex, selectionAnchorIndex);
        return;
      }

      paintCell(cell, eraseMode);
    });

    cell.addEventListener("pointerenter", () => {
      if (isSelecting && selectionAnchorIndex !== null) {
        updateRectSelection(selectionAnchorIndex, Number(cell.dataset.index));
        return;
      }

      if (isPointerDown && !isSelectMode) {
        paintCell(cell, eraseMode);
      }
    });

    cell.addEventListener("pointerup", () => {
      isPointerDown = false;
      isSelecting = false;
    });

    gridEl.appendChild(cell);
  }
}

function parseMatrixSize(value) {
  const [width, height] = value.split("x").map((part) => Number(part));

  if (
    !Number.isInteger(width) ||
    !Number.isInteger(height) ||
    width <= 0 ||
    height <= 0
  ) {
    throw new Error("Invalid matrix size.");
  }

  return { width, height };
}

function setMatrixSize(width, height, keepStatus) {
  stopAnimationPreview();
  const sizeValue = getMatrixSizeValue(width, height);

  if (matrixSizeInput.value !== sizeValue) {
    matrixSizeInput.value = sizeValue;
  }

  if (matrixWidth === width && matrixHeight === height) {
    if (!keepStatus) {
      return;
    }
    setStatus(`Grid resized to ${matrixWidth}x${matrixHeight}.`, "ok");
    return;
  }

  matrixWidth = width;
  matrixHeight = height;
  resetAnimationFrames();
  framePixels = createEmptyPixels(width, height);
  if (currentView === "frame") pixels = framePixels;
  createGrid();
  applyPixels(pixels);

  if (keepStatus) {
    setStatus(`Grid resized to ${matrixWidth}x${matrixHeight}.`, "ok");
  }
}

function resizeMatrix() {
  const { width, height } = parseMatrixSize(matrixSizeInput.value);
  setMatrixSize(width, height, true);
}

document.addEventListener("pointerup", () => {
  isPointerDown = false;
  isSelecting = false;
});

matrixSizeInput.addEventListener("change", resizeMatrix);
clearButton.addEventListener("click", clearGrid);
saveButton.addEventListener("click", saveDrawing);
loadButton.addEventListener("click", loadDrawing);
exportButton.addEventListener("click", exportDrawing);
importButton.addEventListener("click", () => importInput.click());
importInput.addEventListener("change", () =>
  importDrawing(importInput.files[0]),
);
imageImportButton.addEventListener("click", () => imageInput.click());
imageInput.addEventListener("change", () => importImage(imageInput.files[0]));
animationFrameInput.addEventListener("change", () =>
  setActiveAnimationFrame(Number(animationFrameInput.value)),
);
addFrameButton.addEventListener("click", addAnimationFrame);
duplicateFrameButton.addEventListener("click", duplicateAnimationFrame);
deleteFrameButton.addEventListener("click", deleteAnimationFrame);
clearAnimationButton.addEventListener("click", clearAnimation);
selectButton.addEventListener("click", () => setSelectMode(!isSelectMode));
clearSelectionButton.addEventListener("click", () => {
  clearSelection();
  setStatus("Selection cleared.", "ok");
});
shiftLeftButton.addEventListener("click", () => shiftCurrentFrame(-1, 0));
shiftRightButton.addEventListener("click", () => shiftCurrentFrame(1, 0));
shiftUpButton.addEventListener("click", () => shiftCurrentFrame(0, -1));
shiftDownButton.addEventListener("click", () => shiftCurrentFrame(0, 1));
previewAnimationButton.addEventListener("click", toggleAnimationPreview);
sendAnimationButton.addEventListener("click", sendAnimation);
exportAnimationButton.addEventListener("click", exportAnimation);
importAnimationButton.addEventListener("click", () =>
  animationImportInput.click(),
);
animationImportInput.addEventListener("change", () =>
  importAnimation(animationImportInput.files[0]),
);
sendButton.addEventListener("click", sendFrame);
demoButton.addEventListener("click", enableDemo);
textButton.addEventListener("click", sendText);
textInput.addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    sendText();
  }
});
brightnessInput.addEventListener("input", updateBrightnessValue);
brightnessInput.addEventListener("change", sendBrightness);

function setupViews() {
  const groups = document.querySelectorAll(".toolbar-group");
  groups[0].dataset.views = "text";
  groups[1].dataset.views = "frame animation";
  groups[2].dataset.views = "animation";
  groups[3].dataset.views = "settings";
  const demoSection = document.createElement("div");
  demoSection.className = "toolbar-group";
  demoSection.dataset.views = "demo";
  demoSection.appendChild(demoButton);
  const scanner = document.createElement("button");
  scanner.type = "button";
  scanner.textContent = "SCANNER";
  scanner.addEventListener("click", async () => {
    try {
      const response = await fetch("/animation/demo", { method: "POST" });
      if (!response.ok) throw new Error(await response.text());
      setStatus("Scanner started.", "ok");
    } catch (error) { setStatus(error.message, "error"); }
  });
  demoSection.appendChild(scanner);
  document.querySelector(".toolbar").appendChild(demoSection);
  const textColorLabel = document.createElement("label");
  textColorLabel.textContent = "Color";
  const textColor = document.createElement("input");
  textColor.type = "color";
  textColor.id = "textColorInput";
  textColor.value = "#2020ff";
  textColorLabel.appendChild(textColor);
  groups[0].insertBefore(textColorLabel, textButton);
  const tools = document.createElement("select");
  tools.id = "drawingTool";
  tools.setAttribute("aria-label", "Drawing tool");
  for (const [value, label] of [["draw", "Draw"], ["erase", "Erase"], ["select", "Select"]]) {
    const option = document.createElement("option");
    option.value = value; option.textContent = label; tools.appendChild(option);
  }
  tools.addEventListener("change", () => {
    drawingTool = tools.value;
    setSelectMode(drawingTool === "select");
  });
  groups[1].insertBefore(tools, clearButton);
  selectButton.hidden = true;
  document.querySelector(".side").remove();
  matrixSizeInput.disabled = true;
}

function showView() {
  stopAnimationPreview();
  clearSelection();
  const requested = location.hash.replace(/^#\//, "") || "home";
  currentView = ["frame", "text", "animation", "demo", "settings"].includes(requested) ? requested : "home";
  pixels = currentView === "frame" ? framePixels : animationFrames[activeFrameIndex];
  document.getElementById("homeView").hidden = currentView !== "home";
  document.getElementById("backButton").hidden = currentView === "home";
  document.getElementById("viewTitle").textContent = currentView[0].toUpperCase() + currentView.slice(1);
  document.querySelector(".toolbar").hidden = currentView === "home";
  for (const group of document.querySelectorAll("[data-views]")) {
    group.hidden = !group.dataset.views.split(" ").includes(currentView);
  }
  document.querySelector(".editor").hidden = !["frame", "animation"].includes(currentView);
  sendButton.hidden = currentView !== "frame";
  renderPixelsToGrid(pixels);
}

setupViews();
window.addEventListener("hashchange", showView);
gridEl.addEventListener("pointermove", (event) => {
  if (!isPointerDown) return;
  const cell = document.elementFromPoint(event.clientX, event.clientY);
  if (!cell || !cell.classList.contains("cell")) return;
  if (isSelecting && selectionAnchorIndex !== null) {
    updateRectSelection(selectionAnchorIndex, Number(cell.dataset.index));
  } else if (!isSelectMode) paintCell(cell, eraseMode);
});
document.addEventListener("pointercancel", () => { isPointerDown = false; isSelecting = false; });
updateBrightnessValue();
createGrid();
updateAnimationFrameSelector();
showView();
loadDisplayConfig().catch((error) => {
  setStatus(`Failed to load display config: ${error.message}`, "error");
});
