type t = {
  id: string,
  name: string,
  avatarUrl: string,
  title: string,
};

let decode = json =>
  Json.Decode.{
    id: json |> field("id", string),
    name: json |> field("name", string),
    avatarUrl: json |> field("avatarUrl", string),
    title: json |> field("title", string),
  };

let name = t => t.name;

let avatarUrl = t => t.avatarUrl;

let title = t => t.title;

let user = (id, users) =>
  users
  |> ListUtils.unsafeFind(
       user => user.id == id,
       "Unable to find user with id " ++ id ++ "in QuestionShow__UserData",
     );

let userName = (id, users) => user(id, users) |> name;

let userAvatarUrl = (id, users) => user(id, users) |> avatarUrl;
