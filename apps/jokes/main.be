import json

var root = ui.root()

# Joke text label
var joke_lbl = ui.label(root, "")

# Check platform support
if !http.supported()
    ui.set_text(joke_lbl, "HTTP not supported\non this platform")
else
    ui.set_text(joke_lbl, "Fetching...")

    var resp = http.get("https://v2.jokeapi.dev/joke/Any?safe-mode")
    var code = resp["status"]

    if code != 200
        var errmsg = resp["body"]
        if !errmsg errmsg = "Unknown error" end
        ui.set_text(joke_lbl, "Error: " + errmsg)
    else
        var data = json.load(resp["body"])
        if !data
            ui.set_text(joke_lbl, "Could not parse response")
        else
            var text = ""
            if data["type"] == "single"
                text = data["joke"]
            else
                text = data["setup"] + "\n\n" + data["delivery"]
            end
            ui.set_text(joke_lbl, text)
        end
    end
end
