class Computador < ApplicationRecord
  belongs_to :pessoa
  
  validates :numserie, :marca, presence: true
end
