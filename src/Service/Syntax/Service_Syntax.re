module Core = Oni_Core;
module Ext = Oni_Extensions;
module OptionEx = Core.Utility.OptionEx;

[@deriving show({with_path: false})]
type msg =
  | ServerStarted([@opaque] Oni_Syntax_Client.t)
  | ServerFailedToStart(string)
  | ServerClosed
  | ReceivedHighlights([@opaque] list(Oni_Syntax.Protocol.TokenUpdate.t));

module Sub = {
  type params = {
    id: string,
    languageInfo: Ext.LanguageInfo.t,
    setup: Core.Setup.t,
  };

  module SyntaxSubscription =
    Isolinear.Sub.Make({
      type nonrec msg = msg;

      type nonrec params = params;

      type state = result(Oni_Syntax_Client.t, string);

      let name = "SyntaxSubscription";
      let id = params => params.id;

      let init = (~params, ~dispatch) => {
        let clientResult =
          Oni_Syntax_Client.start(
            ~onClose=_ => dispatch(ServerClosed),
            ~onHighlights=
              highlights => {dispatch(ReceivedHighlights(highlights))},
            ~onHealthCheckResult=_ => (),
            params.languageInfo,
            params.setup,
          )
          |> Utility.ResultEx.tap(client => dispatch(ServerStarted(client)))
          |> Utility.ResultEx.tapError(msg =>
               dispatch(ServerFailedToStart(msg))
             );

        clientResult;
      };

      let update = (~params as _, ~state, ~dispatch as _) => state;

      let dispose = (~params as _, ~state) => {
        state |> Result.iter(Oni_Syntax_Client.close);
      };
    });

  let create = (~languageInfo, ~setup) => {
    SyntaxSubscription.create({id: "syntax-highligher", languageInfo, setup});
  };
};

module Effect = {
  let bufferEnter = (maybeSyntaxClient, id: int, fileType) =>
    Isolinear.Effect.create(~name="syntax.bufferEnter", () => {
      OptionEx.iter2(
        (syntaxClient, fileType) => {
          Oni_Syntax_Client.notifyBufferEnter(syntaxClient, id, fileType)
        },
        maybeSyntaxClient,
        fileType,
      )
    });

  let bufferUpdate =
      (
        maybeSyntaxClient,
        bufferUpdate: Oni_Core.BufferUpdate.t,
        lines,
        scopeMaybe,
      ) =>
    Isolinear.Effect.create(~name="syntax.bufferUpdate", () => {
      OptionEx.iter2(
        (syntaxClient, scope) => {
          Oni_Syntax_Client.notifyBufferUpdate(
            syntaxClient,
            bufferUpdate,
            lines,
            scope,
          )
        },
        maybeSyntaxClient,
        scopeMaybe,
      )
    });

  let configurationChange = (maybeSyntaxClient, config: Core.Configuration.t) =>
    Isolinear.Effect.create(~name="syntax.configurationChange", () => {
      Option.iter(
        syntaxClient =>
          Oni_Syntax_Client.notifyConfigurationChanged(syntaxClient, config),
        maybeSyntaxClient,
      )
    });

  let themeChange = (maybeSyntaxClient, theme) =>
    Isolinear.Effect.create(~name="syntax.theme", () => {
      Option.iter(
        syntaxClient => {
          Oni_Syntax_Client.notifyThemeChanged(syntaxClient, theme)
        },
        maybeSyntaxClient,
      )
    });

  let visibilityChanged = (maybeSyntaxClient, visibleRanges) =>
    Isolinear.Effect.create(~name="syntax.visibilityChange", () => {
      Option.iter(
        syntaxClient =>
          Oni_Syntax_Client.notifyVisibilityChanged(
            syntaxClient,
            visibleRanges,
          ),
        maybeSyntaxClient,
      )
    });
};
