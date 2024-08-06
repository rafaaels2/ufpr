class Computador < ApplicationRecord
  belongs_to :pessoa

  validates :numserie, :marca, presence: true
  validates :pessoa_id, presence: true
end

