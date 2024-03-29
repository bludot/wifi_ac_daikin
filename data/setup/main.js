function GetAccessPoints() {
    return fetch("/setup/scan").then((e => e.json())).then()
}

function ScanAndDisplay() {
    GetAccessPoints().then((e => {
        document.querySelector("#accessPoints ul").innerHTML = "", e.accessPoints.filter((e => e.ssid && e.security && e.mac)).map((e => {
            const t = document.createElement("li"), n = document.createElement("span");
            n.className = "name", n.appendChild(document.createTextNode(e.ssid));
            const c = document.createElement("span");
            c.className = "security", c.appendChild(document.createTextNode(e.security));
            const o = document.createElement("span");
            o.className = "mac", o.appendChild(document.createTextNode(e.mac)), t.appendChild(n), t.appendChild(o), t.appendChild(c), t.addEventListener("click", (function (t) {
                console.log(this.querySelector(".name").innerText), "NONE" != e.security ? Prompt(e) : fetch("/setup/connect", {
                    method: "POST",
                    body: JSON.stringify(connectData)
                })
            })), document.querySelector("#accessPoints ul").appendChild(t)
        }))
    }))
}

function Prompt(e) {
    var t = document.createElement("pure-dialog");
    t.id = "example", t.title = "Pure Dialog Demo", t.content = '\n <label for="password">Password:</label>\n \t<input type="password" name="password"/>\n ', t.buttons = "OK, Cancel", t.buttonValueSeparator = ",", t.closeButton = !1, t.addEventListener("pure-dialog-button-clicked", (function (n) {
        if ("OK" === n.detail) {
            console.log("clicked ok"), console.log(e);
            const n = {ssid: e.ssid, password: document.querySelector('input[name="password"]').value};
            fetch("/setup/connect", {method: "POST", body: JSON.stringify(n)}), t.remove()
        } else console.log("canceled"), t.remove()
    })), t.appendToDOM(), t.showModal()
}