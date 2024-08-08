class Carro < ApplicationRecord
  belongs_to :pessoa
  
  validates :crv, :nome, :marca, presence: true
  validate :validate_unique_car_per_person
    
  private

  def validate_unique_car_per_person
    if Carro.exists?(pessoa_id: pessoa_id)
      errors.add(:pessoa, "já possui um carro")
    end
  end
end

