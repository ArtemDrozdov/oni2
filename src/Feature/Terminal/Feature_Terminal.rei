open Oni_Core;

module ExtHostClient = Oni_Extensions.ExtHostClient;

type terminal = {
  id: int,
  cmd: string,
  rows: int,
  columns: int,
  pid: option(int),
  title: option(string),
};

type t;

let initial: t;

let toList: t => list((int, terminal));

let getTerminalOpt: (int, t) => option(terminal);

type splitDirection =
  | Vertical
  | Horizontal;

type msg =
  | Started({splitDirection})
  | Service(Service_Terminal.msg);

type outmsg =
  | Nothing
  | OpenBuffer({
      name: string,
      splitDirection,
    });

let update: (ExtHostClient.t, t, msg) => (t, outmsg);

let subscription:
  (~workspaceUri: Uri.t, ExtHostClient.t, t) => Isolinear.Sub.t(msg);

let shellCmd: string;