// Simple JS to log a message when the page loads
document.addEventListener("DOMContentLoaded", () => {
    console.log("Hello from script.js!");
    const h1 = document.querySelector("h1");
    h1.textContent += " (Modified by JS)";
});
