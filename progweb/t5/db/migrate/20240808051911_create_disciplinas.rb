class CreateDisciplinas < ActiveRecord::Migration[7.1]
  def change
    create_table :disciplinas do |t|
      t.string :codigo
      t.string :nome

      t.timestamps
    end
  end
end
