local moo = import "moo.jsonnet";
local ns = "dunedaq.dfbackend.trdispatcher";
local s = moo.oschema.schema(ns);

local types = {
    size: s.number("Size", "u8",
                   doc="A count of very many things"),

    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    conf: s.record("ConfParams", [
        s.field("nIntsPerList", self.size, 4,
                doc="Number of numbers"),
        s.field("waitBetweenSendsMsec", self.count, 1000,
                doc="Millisecs to wait between sending"),
    ], doc="trdispatcher configuration"),

};

moo.oschema.sort_select(types, ns)
