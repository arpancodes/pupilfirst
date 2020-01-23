[@bs.config {jsx: 3}];

exception FormNotFound(string);

[%bs.raw {|require("./CurriculumEditor__ContentBlockCreator.css")|}];

open CurriculumEditor__Types;

let str = React.string;

module CreateMarkdownContentBlock = [%graphql
  {|
    mutation($targetId: ID!, $aboveContentBlockId: ID) {
      createMarkdownContentBlock(targetId: $targetId, aboveContentBlockId: $aboveContentBlockId) {
        contentBlock {
          ...ContentBlock.Fragments.AllFields
        }
      }
    }
  |}
];

type ui =
  | Hidden
  | BlockSelector
  | EmbedForm(string);

type state = {
  ui,
  saving: bool,
  error: option(string),
};

type action =
  | ToggleVisibility
  | ToggleSaving
  | FinishSaving(bool)
  | SetError(string)
  | FailToUpload
  | ShowEmbedForm
  | HideEmbedForm
  | UpdateEmbedUrl(string);

let computeInitialState = isAboveTarget => {
  {ui: isAboveTarget ? Hidden : BlockSelector, saving: false, error: None};
};

let reducer = (state, action) =>
  switch (action) {
  | ToggleVisibility =>
    let ui =
      switch (state.ui) {
      | Hidden => BlockSelector
      | BlockSelector
      | EmbedForm(_) => Hidden
      };

    {...state, ui};
  | ToggleSaving => {...state, saving: !state.saving, error: None}
  | FinishSaving(isAboveTarget) => computeInitialState(isAboveTarget)
  | SetError(error) => {...state, error: Some(error)}
  | FailToUpload => {
      ...state,
      saving: false,
      error:
        Some(
          "Failed to upload file. Please check message in notification, and try again.",
        ),
    }
  | ShowEmbedForm => {...state, ui: EmbedForm("")}
  | HideEmbedForm => {...state, ui: BlockSelector}
  | UpdateEmbedUrl(url) => {...state, ui: EmbedForm(url)}
  };

let containerClasses = (visible, isAboveTarget) => {
  let classes = "content-block-creator py-3";
  classes ++ (visible || !isAboveTarget ? " content-block-creator--open" : "");
};

let createMarkdownContentBlock =
    (target, aboveContentBlock, send, addContentBlockCB) => {
  send(ToggleSaving);
  let aboveContentBlockId =
    aboveContentBlock |> OptionUtils.map(ContentBlock.id);
  let targetId = target |> Target.id;
  CreateMarkdownContentBlock.make(~targetId, ~aboveContentBlockId?, ())
  |> GraphqlQuery.sendQuery2
  |> Js.Promise.then_(result => {
       switch (result##createMarkdownContentBlock##contentBlock) {
       | Some(contentBlock) =>
         contentBlock |> ContentBlock.makeFromJs |> addContentBlockCB;
         send(FinishSaving(aboveContentBlock != None));
       | None => send(ToggleSaving)
       };

       Js.Promise.resolve();
     })
  |> ignore;
};

let elementId = (prefix, aboveContentBlock) =>
  prefix
  ++ (
    switch (aboveContentBlock) {
    | Some(contentBlock) => contentBlock |> ContentBlock.id
    | None => "bottom"
    }
  );

let fileInputId = aboveContentBlock =>
  aboveContentBlock |> elementId("markdown-block-file-input-");
let imageInputId = aboveContentBlock =>
  aboveContentBlock |> elementId("markdown-block-image-input-");
let fileFormId = aboveContentBlock =>
  aboveContentBlock |> elementId("markdown-block-file-form-");
let imageFormId = aboveContentBlock =>
  aboveContentBlock |> elementId("markdown-block-image-form-");

let onBlockTypeSelect =
    (target, aboveContentBlock, send, addContentBlockCB, blockType, event) => {
  // event |> ReactEvent.Mouse.preventDefault;
  switch (blockType) {
  | `Markdown =>
    createMarkdownContentBlock(
      target,
      aboveContentBlock,
      send,
      addContentBlockCB,
    )
  | `File => ()
  | `Image => ()
  | `Embed => send(ShowEmbedForm)
  };
};

let button = (target, aboveContentBlock, send, addContentBlockCB, blockType) => {
  let fileId = aboveContentBlock |> fileInputId;
  let imageId = aboveContentBlock |> imageInputId;

  let (faIcon, buttonText, htmlFor) =
    switch (blockType) {
    | `Markdown => ("fab fa-markdown", "Markdown", None)
    | `File => ("far fa-file-alt", "File", Some(fileId))
    | `Image => ("far fa-image", "Image", Some(imageId))
    | `Embed => ("fas fa-code", "Embed", None)
    };

  <label
    ?htmlFor
    key=buttonText
    className="content-block-creator__block-content-type-picker px-3 pt-4 pb-3 flex-1 text-center text-primary-200"
    onClick={onBlockTypeSelect(
      target,
      aboveContentBlock,
      send,
      addContentBlockCB,
      blockType,
    )}>
    <i className={faIcon ++ " text-2xl"} />
    <p className="font-semibold"> {buttonText |> str} </p>
  </label>;
};

let maxAllowedFileSize = 5 * 1024 * 1024;
let isInvalidFile = file => file##size > maxAllowedFileSize;

let isInvalidImageFile = image =>
  (
    switch (image##_type) {
    | "image/jpeg"
    | "image/gif"
    | "image/png" => false
    | _ => true
    }
  )
  || image
  |> isInvalidFile;

let uploadFile =
    (target, send, addContentBlockCB, isAboveContentBlock, formData) =>
  Api.sendFormData(
    "/school/targets/" ++ (target |> Target.id) ++ "/content_block",
    formData,
    json => {
      Notification.success("Done!", "File uploaded successfully.");
      let contentBlock = json |> ContentBlock.decode;
      addContentBlockCB(contentBlock);
      send(FinishSaving(isAboveContentBlock));
    },
    () => send(FailToUpload),
  );

let submitForm =
    (target, aboveContentBlock, send, addContentBlockCB, blockType) => {
  let formId =
    switch (blockType) {
    | `File => fileFormId(aboveContentBlock)
    | `Image => imageFormId(aboveContentBlock)
    };

  let element = ReactDOMRe._getElementById(formId);

  switch (element) {
  | Some(element) =>
    DomUtils.FormData.create(element)
    |> uploadFile(target, send, addContentBlockCB, aboveContentBlock != None)
  | None =>
    Rollbar.error(
      "Could not find form to upload file for content block: " ++ formId,
    );
    raise(FormNotFound(formId));
  };
};

let handleFileInputChange =
    (target, aboveContentBlock, send, addContentBlockCB, blockType, event) => {
  event |> ReactEvent.Form.preventDefault;

  switch (ReactEvent.Form.target(event)##files) {
  | [||] => ()
  | files =>
    let file = files[0];

    let error =
      switch (blockType) {
      | `File =>
        file |> isInvalidFile
          ? Some("Please select a file with a size less than 5 MB.") : None
      | `Image =>
        file |> isInvalidImageFile
          ? Some(
              "Please select an image (PNG, JPEG, GIF) with a size less than 5 MB, and less than 4096px wide or high.",
            )
          : None
      };

    switch (error) {
    | Some(error) => send(SetError(error))
    | None =>
      // let filename = file##name;
      send(ToggleSaving);
      submitForm(
        target,
        aboveContentBlock,
        send,
        addContentBlockCB,
        blockType,
      );
    };
  };
};

let uploadForm =
    (target, aboveContentBlock, send, addContentBlockCB, blockType) => {
  let fileSelectionHandler =
    handleFileInputChange(target, aboveContentBlock, send, addContentBlockCB);

  let (fileId, formId, onChange, fileType) =
    switch (blockType) {
    | `File => (
        fileInputId(aboveContentBlock),
        fileFormId(aboveContentBlock),
        fileSelectionHandler(`File),
        "file",
      )
    | `Image => (
        imageInputId(aboveContentBlock),
        imageFormId(aboveContentBlock),
        fileSelectionHandler(`Image),
        "image",
      )
    };

  <form className="hidden" id=formId>
    <input
      name="authenticity_token"
      type_="hidden"
      value={AuthenticityToken.fromHead()}
    />
    <input type_="hidden" name="block_type" value=fileType />
    <input
      type_="file"
      name="file"
      id=fileId
      onChange
      required=true
      multiple=false
    />
    {switch (aboveContentBlock) {
     | Some(contentBlock) =>
       <input
         type_="hidden"
         name="above_content_block_id"
         value={contentBlock |> ContentBlock.id}
       />
     | None => React.null
     }}
  </form>;
};

let visible = state =>
  switch (state.ui) {
  | Hidden => false
  | BlockSelector
  | EmbedForm(_) => true
  };

let updateEmbedUrl = (send, event) => {
  let value = ReactEvent.Form.target(event)##value;
  send(UpdateEmbedUrl(value));
};

let creatorToggler = (state, send, contentBlock) => {
  let (action, icon) =
    switch (state.ui) {
    | Hidden
    | BlockSelector => (
        ToggleVisibility,
        "fa-plus content-block-creator__plus-button-icon",
      )
    | EmbedForm(_) => (HideEmbedForm, "fa-undo-alt")
    };

  <div
    className="content-block-creator__plus-button-container relative cursor-pointer"
    onClick={_event => send(action)}>
    <div
      id={"add-block-above-" ++ (contentBlock |> ContentBlock.id)}
      title="Add block"
      className="content-block-creator__plus-button bg-gray-200 hover:bg-gray-300 relative rounded-lg border border-gray-500 w-10 h-10 flex justify-center items-center mx-auto z-20">
      <FaIcon classes={"text-base fas " ++ icon} />
    </div>
  </div>;
};

[@react.component]
let make = (~target, ~aboveContentBlock=?, ~addContentBlockCB) => {
  let isAboveContentBlock = aboveContentBlock != None;

  let (state, send) =
    React.useReducerWithMapState(
      reducer,
      isAboveContentBlock,
      computeInitialState,
    );

  <DisablingCover disabled={state.saving} message="Creating...">
    {uploadForm(target, aboveContentBlock, send, addContentBlockCB, `File)}
    {uploadForm(target, aboveContentBlock, send, addContentBlockCB, `Image)}
    <div className={containerClasses(state |> visible, isAboveContentBlock)}>
      {switch (aboveContentBlock) {
       | Some(contentBlock) => creatorToggler(state, send, contentBlock)
       | None => <div className="h-10" />
       }}
      <div className="content-block-creator__inner-container">
        {switch (state.ui) {
         | Hidden => React.null
         | BlockSelector =>
           <div
             className="content-block-creator__block-content-type text-sm hidden shadow-lg mx-auto relative bg-primary-900 rounded-lg -mt-4 z-10">
             {[|`Markdown, `Image, `Embed, `File|]
              |> Array.map(
                   button(target, aboveContentBlock, send, addContentBlockCB),
                 )
              |> React.array}
           </div>
         | EmbedForm(url) =>
           <div
             className="clearfix border-2 border-gray-400 bg-gray-200 border-dashed rounded-lg px-3 pb-3 pt-2 -mt-4 z-10">
             <label className="text-xs font-semibold">
               {"URL to Embed" |> str}
             </label>
             <div className="flex mt-1">
               <input
                 placeholder="https://www.youtube.com/watch?v="
                 className="w-full mr-2 py-1 px-2 border rounded"
                 type_="text"
                 value=url
                 onChange={updateEmbedUrl(send)}
               />
               <div>
                 <button className="btn btn-success"> {"Save" |> str} </button>
               </div>
             </div>
           </div>
         }}
      </div>
      {switch (state.error) {
       | Some(error) =>
         <School__InputGroupError message=error active={state |> visible} />
       | None => React.null
       }}
    </div>
  </DisablingCover>;
};
