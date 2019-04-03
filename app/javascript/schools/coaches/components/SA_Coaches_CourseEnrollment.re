open CoachesCourseEnrollment__Types;

/* open SchoolAdmin__Utils; */
let str = ReasonReact.string;

type formVisible =
  | None
  | CoachEnrollmentForm;

type state = {
  courseCoaches: list(Coach.t),
  teamCoaches: list(Coach.t),
  formVisible,
};

type action =
  | UpdateFormVisible(formVisible)
  | UpdateCoaches(list(int));

let component = ReasonReact.reducerComponent("SA_Coaches_CourseEnrollment");

let make =
    (
      ~courseCoachIds,
      ~startupCoachIds,
      ~schoolCoaches,
      ~courseId,
      ~authenticityToken,
      _children,
    ) => {
  ...component,
  initialState: () => {
    courseCoaches:
      schoolCoaches
      |> List.filter(schoolCoach =>
           courseCoachIds
           |> List.exists(facultyId => facultyId == Coach.id(schoolCoach))
         ),
    teamCoaches:
      schoolCoaches
      |> List.filter(schoolCoach =>
           startupCoachIds
           |> List.exists(facultyId => facultyId == Coach.id(schoolCoach))
         ),
    formVisible: None,
  },
  reducer: (action, state) =>
    switch (action) {
    | UpdateFormVisible(formVisible) =>
      ReasonReact.Update({...state, formVisible})
    | UpdateCoaches(coachIds) =>
      let newCoachesList =
        schoolCoaches
        |> List.filter(schoolCoach =>
             coachIds
             |> List.exists(coachId => coachId == Coach.id(schoolCoach))
           );
      let newTeamCoaches =
        state.teamCoaches
        |> List.filter(teamCoach =>
             ! (
               newCoachesList |> List.exists(newCoach => newCoach == teamCoach)
             )
           );
      ReasonReact.Update({
        ...state,
        courseCoaches: newCoachesList,
        teamCoaches: newTeamCoaches,
      });
    },
  render: ({state, send}) => {
    let closeFormCB = () => send(UpdateFormVisible(None));
    let updateCoachesCB = coachIds => {
      send(UpdateCoaches(coachIds));
      send(UpdateFormVisible(None));
    };
    <div className="flex flex-1 h-screen">
      (
        switch (state.formVisible) {
        | None => ReasonReact.null
        | CoachEnrollmentForm =>
          let courseCoachIds =
            state.courseCoaches |> List.map(coach => coach |> Coach.id);
          <SA_Coaches_CourseEnrollmentForm
            courseId
            courseCoachIds
            schoolCoaches
            updateCoachesCB
            closeFormCB
            authenticityToken
          />;
        }
      )
      <div className="flex-1 flex flex-col bg-grey-lightest overflow-hidden">
        <div
          className="flex px-6 py-2 items-center justify-between overflow-y-scroll">
          <button
            onClick=(
              _event => {
                ReactEvent.Mouse.preventDefault(_event);
                send(UpdateFormVisible(CoachEnrollmentForm));
              }
            )
            className="max-w-md w-full flex mx-auto items-center justify-center relative bg-grey-lighter hover:bg-grey-light hover:shadow-md border-2 border-dashed p-6 rounded-lg mt-12 cursor-pointer">
            <i className="material-icons"> ("add_circle_outline" |> str) </i>
            <h4 className="font-semibold ml-2">
              ("Assign/Remove Course Faculty" |> str)
            </h4>
          </button>
        </div>
        (
          state.teamCoaches
          |> ListUtils.isEmpty
          && state.courseCoaches
          |> ListUtils.isEmpty ?
            <div
              className="flex justify-center bg-grey-lightest border rounded p-3 italic mt-2">
              ("The course has no coaches assigned!" |> str)
            </div> :
            ReasonReact.null
        )
        <div
          className="px-6 pb-4 mt-5 flex flex-1 bg-grey-lightest overflow-y-scroll">
          <div className="max-w-md w-full mx-auto relative">
            (
              state.courseCoaches |> ListUtils.isEmpty ?
                ReasonReact.null :
                <div className="mb-5 w-full"> ("Course Coaches:" |> str) </div>
            )
            (
              state.courseCoaches
              |> List.sort((x, y) => (x |> Coach.id) - (y |> Coach.id))
              |> List.map(coach =>
                   <div
                     key=(coach |> Coach.id |> string_of_int)
                     className="flex items-center shadow bg-white rounded-lg overflow-hidden mb-4">
                     <div
                       className="course-faculty__list-item flex w-full hover:bg-grey-lighter">
                       <div className="flex flex-1 items-center py-4 px-4">
                         <img
                           className="w-10 h-10 rounded-full mr-4"
                           src=(coach |> Coach.imageUrl)
                           alt="Avatar of Jonathan Reinink"
                         />
                         <div className="text-sm">
                           <p className="text-black font-semibold">
                             (coach |> Coach.name |> str)
                           </p>
                           <p
                             className="text-grey-dark font-semibold text-xs mt-1">
                             (coach |> Coach.title |> str)
                           </p>
                         </div>
                       </div>
                     </div>
                   </div>
                 )
              |> Array.of_list
              |> ReasonReact.array
            )
            (
              state.teamCoaches |> ListUtils.isEmpty ?
                ReasonReact.null :
                <div className="mb-5 w-full">
                  ("Student/Team Coaches:" |> str)
                </div>
            )
            (
              state.teamCoaches
              |> List.sort((x, y) => (x |> Coach.id) - (y |> Coach.id))
              |> List.map(coach =>
                   <div
                     key=(coach |> Coach.id |> string_of_int)
                     className="flex items-center shadow bg-white rounded-lg overflow-hidden mb-4">
                     <div
                       className="course-faculty__list-item flex w-full hover:bg-grey-lighter">
                       <div className="flex flex-1 items-center py-4 px-4">
                         <img
                           className="w-10 h-10 rounded-full mr-4"
                           src=(coach |> Coach.imageUrl)
                           alt=("Avatar of " ++ Coach.name(coach))
                         />
                         <div className="text-sm">
                           <p className="text-black font-semibold">
                             (coach |> Coach.name |> str)
                           </p>
                           <p
                             className="text-grey-dark font-semibold text-xs mt-1">
                             (coach |> Coach.title |> str)
                           </p>
                           (
                             switch (coach |> Coach.teams) {
                             | None => ReasonReact.null
                             | Some(teams) =>
                               <p
                                 className="text-grey-dark font-semibold text-xs mt-1">
                                 (
                                   "Teams: "
                                   ++ (
                                     List.map(team => Team.name(team), teams)
                                     |> String.concat(" | ")
                                   )
                                   |> str
                                 )
                               </p>
                             }
                           )
                         </div>
                       </div>
                     </div>
                   </div>
                 )
              |> Array.of_list
              |> ReasonReact.array
            )
          </div>
        </div>
      </div>
    </div>;
  },
};

type props = {
  courseCoachIds: list(int),
  startupCoachIds: list(int),
  schoolCoaches: list(Coach.t),
  authenticityToken: string,
  courseId: int,
};

let decode = json =>
  Json.Decode.{
    courseCoachIds: json |> field("courseCoachIds", list(int)),
    startupCoachIds: json |> field("startupCoachIds", list(int)),
    schoolCoaches: json |> field("schoolCoaches", list(Coach.decode)),
    courseId: json |> field("courseId", int),
    authenticityToken: json |> field("authenticityToken", string),
  };

let jsComponent =
  ReasonReact.wrapReasonForJs(
    ~component,
    jsProps => {
      let props = jsProps |> decode;
      make(
        ~courseCoachIds=props.courseCoachIds,
        ~startupCoachIds=props.startupCoachIds,
        ~schoolCoaches=props.schoolCoaches,
        ~courseId=props.courseId,
        ~authenticityToken=props.authenticityToken,
        [||],
      );
    },
  );