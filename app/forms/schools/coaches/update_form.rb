module Schools
  module Coaches
    class UpdateForm < Reform::Form
      property :email, validates: { email: true }, virtual: true
      property :name, validates: { presence: true, length: { maximum: 250 } }
      property :title, validates: { presence: true, length: { maximum: 250 } }
      property :linkedin_url
      property :connect_link
      property :notify_for_submission
      property :public
      property :image, virtual: true, validates: { file_content_type: { allow: ['image/jpeg', 'image/png'] }, file_size: { less_than: 2.gigabytes } }
      property :school_id, virtual: true, validates: { presence: true }

      def save
        Faculty.transaction do
          user = User.where(email: email).first_or_create!
          model.update!(faculty_params.merge(user: user))
          model.image.attach(image) if image.present?
        end

        model
      end

      private

      def school
        School.find_by(id: school_id)
      end

      def faculty_params
        {
          name: name,
          title: title,
          school: school,
          linkedin_url: linkedin_url,
          connect_link: connect_link,
          public: public,
          notify_for_submission: notify_for_submission
        }
      end

      def faculty
        @faculty ||= Faculty.find_by(id: id)
      end
    end
  end
end